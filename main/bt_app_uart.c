#include "bt_app_uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"

#define UART_RX_BUF_SIZE 1028
#define UART_TX_BUF_SIZE 1028
#define UART_BAUD_RATE 115200


/* Static function definitions */

static void bt_uart_task_handler(void *arg)
{

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

    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE, 0, NULL, NULL));
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, CONFIG_UART_DATA_TX_PIN, CONFIG_UART_DATA_RX_PIN, 
                    UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

void bt_uart_driver_uninstall()
{
    ESP_ERROR_CHECK(uart_driver_delete(UART_NUM_0));
}

void bt_uart_task_start()
{

}

void bt_uart_task_stop()
{

}

size_t bt_uart_async_send(const char *data, int len)
{

}