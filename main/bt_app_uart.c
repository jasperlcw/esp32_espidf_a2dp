#include "bt_app_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"

#define UART_RX_BUF_SIZE 512
#define UART_TX_BUF_SIZE 512
#define UART_BAUD_RATE 115200
#define UART_RING_BUF_SIZE 512

/* Static variable declarations */

static TaskHandle_t uart_task_handle = NULL;
static RingbufHandle_t uart_ring_buf_handle = NULL;

/* Static function definitions */

static void bt_uart_task_handler(void *arg)
{
    size_t item_size = 0;
    char *to_send;
    while(1) {
        item_size = 0;
        to_send = xRingbufferReceive(uart_ring_buf_handle, &item_size, 2000);
        if (to_send != NULL) {
            int bytes_sent = uart_write_bytes(UART_NUM_0, to_send, item_size);
            if (!bytes_sent) {
                ESP_LOGI(BT_UART_TAG, "Unable to send data through the UART bus.");
            }
            vRingbufferReturnItem(uart_ring_buf_handle, to_send);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

/* Public function definitions */

void bt_uart_driver_install()
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, CONFIG_UART_DATA_TX_PIN, CONFIG_UART_DATA_RX_PIN, 
                    UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void bt_uart_driver_uninstall()
{
    ESP_ERROR_CHECK(uart_driver_delete(UART_NUM_0));
}

esp_err_t bt_uart_task_start()
{
    uart_ring_buf_handle = xRingbufferCreate(UART_RING_BUF_SIZE, RINGBUF_TYPE_NOSPLIT);
    if (uart_ring_buf_handle == NULL) {
        ESP_LOGE(BT_UART_TAG, "Failed to create ring buffer for UART TX data.");
        return ESP_FAIL;
    }
    ESP_LOGI(BT_UART_TAG, "Starting UART task.");
    xTaskCreate(bt_uart_task_handler, "BtUARTTask", 2048, NULL, configMAX_PRIORITIES - 20, &uart_task_handle);
    return ESP_OK;
}

void bt_uart_task_stop()
{
    if (uart_task_handle) {
        vTaskDelete(uart_task_handle);
        uart_task_handle = NULL;
    }
    if (uart_ring_buf_handle) {
        vRingbufferDelete(uart_ring_buf_handle);
        uart_ring_buf_handle = NULL;
    }
}

size_t bt_uart_async_send(const char *data, size_t len)
{
    if (uart_ring_buf_handle == NULL) {
        ESP_LOGI(BT_UART_TAG, "Unable to queue UART data as the task has not been started.");
        return 0;
    }

    BaseType_t queued = pdFALSE;
    queued = xRingbufferSend(uart_ring_buf_handle, data, len, 0);
    if (!queued) {
        ESP_LOGI(BT_UART_TAG, "Unable to queue UART data into the circular buffer.");
        return 0;
    }
    return len;
}