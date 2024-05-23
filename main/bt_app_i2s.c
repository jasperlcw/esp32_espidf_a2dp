#include "bt_app_i2s.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"

#ifdef CONFIG_EXAMPLE_A2DP_SINK_OUTPUT_INTERNAL_DAC
#include "driver/dac_continuous.h"
#else
#include "driver/i2s_std.h"
#endif

#define RINGBUF_MAX_CAPACITY        (32 * 1024)      // 32 KB
#define RINGBUF_PREFETCH_CAPACITY   (20 * 1024)      // 20 KB

typedef enum RingBufMode_t {
    RINGBUFFER_MODE_PROCESSING,    /* ringbuffer is buffering incoming audio data, I2S is working */
    RINGBUFFER_MODE_PREFETCHING,   /* ringbuffer is buffering incoming audio data, I2S is waiting */
    RINGBUFFER_MODE_DROPPING       /* ringbuffer is not buffering (dropping) incoming audio data, I2S is working */
} RingBufMode_t;

/* handler for I2S task */
static void bt_i2s_task_handler(void *arg);

/* Static variable declarations */

#ifndef CONFIG_EXAMPLE_A2DP_SINK_OUTPUT_INTERNAL_DAC
static i2s_chan_handle_t tx_chan = NULL;
#else
static dac_continuous_handle_t tx_chan;
#endif

static TaskHandle_t s_bt_i2s_task_handle = NULL;  /* handle of I2S task */
static RingbufHandle_t s_ringbuf_i2s = NULL;     /* handle of ringbuffer for I2S */
static SemaphoreHandle_t s_i2s_write_semaphore = NULL;
static RingBufMode_t ringbuffer_mode = RINGBUFFER_MODE_PROCESSING;

/* Static function definitions */
static void bt_i2s_task_handler(void *arg)
{
    uint8_t *data = NULL;
    size_t item_size = 0;
    /**
     * The total length of DMA buffer of I2S is:
     * `dma_frame_num * dma_desc_num * i2s_channel_num * i2s_data_bit_width / 8`.
     * Transmit `dma_frame_num * dma_desc_num` bytes to DMA is trade-off.
     */
    const size_t item_size_upto = 240 * 6;
    size_t bytes_written = 0;

    for (;;) {
        if (pdTRUE == xSemaphoreTake(s_i2s_write_semaphore, portMAX_DELAY)) {
            for (;;) {
                item_size = 0;
                /* receive data from ringbuffer and write it to I2S DMA transmit buffer */
                data = (uint8_t *)xRingbufferReceiveUpTo(s_ringbuf_i2s, &item_size, (TickType_t)pdMS_TO_TICKS(20), item_size_upto);
                if (item_size == 0) {
                    ESP_LOGI(BT_I2S_TAG, "ringbuffer underflowed! mode changed: RINGBUFFER_MODE_PREFETCHING");
                    ringbuffer_mode = RINGBUFFER_MODE_PREFETCHING;
                    break;
                }

            #ifdef CONFIG_EXAMPLE_A2DP_SINK_OUTPUT_INTERNAL_DAC
                dac_continuous_write(tx_chan, data, item_size, &bytes_written, -1);
            #else
                i2s_channel_write(tx_chan, data, item_size, &bytes_written, portMAX_DELAY);
            #endif
                vRingbufferReturnItem(s_ringbuf_i2s, (void *)data);
            }
        }
    }
}

/* Public function definitions */

void bt_i2s_driver_install(void)
{
#ifdef CONFIG_EXAMPLE_A2DP_SINK_OUTPUT_INTERNAL_DAC
    dac_continuous_config_t cont_cfg = {
        .chan_mask = DAC_CHANNEL_MASK_ALL,
        .desc_num = 8,
        .buf_size = 2048,
        .freq_hz = 44100,
        .offset = 127,
        .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,   // Using APLL as clock source to get a wider frequency range
        .chan_mode = DAC_CHANNEL_MODE_ALTER,
    };
    /* Allocate continuous channels */
    ESP_ERROR_CHECK(dac_continuous_new_channels(&cont_cfg, &tx_chan));
    /* Enable the continuous channels */
    ESP_ERROR_CHECK(dac_continuous_enable(tx_chan));
#else
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = CONFIG_EXAMPLE_I2S_BCK_PIN,     // default pin is 26
            .ws = CONFIG_EXAMPLE_I2S_LRCK_PIN,      // default pin is 22
            .dout = CONFIG_EXAMPLE_I2S_DATA_PIN,    // default pin is 25
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    /* enable I2S */
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
#endif
}

void bt_i2s_driver_uninstall(void)
{
#ifdef CONFIG_EXAMPLE_A2DP_SINK_OUTPUT_INTERNAL_DAC
    ESP_ERROR_CHECK(dac_continuous_disable(tx_chan));
    ESP_ERROR_CHECK(dac_continuous_del_channels(tx_chan));
#else
    ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
    ESP_ERROR_CHECK(i2s_del_channel(tx_chan));
#endif
}

void bt_i2s_task_start_up(void)
{
    ESP_LOGI(BT_I2S_TAG, "ringbuffer data empty! mode changed: RINGBUFFER_MODE_PREFETCHING");
    ringbuffer_mode = RINGBUFFER_MODE_PREFETCHING;
    if ((s_i2s_write_semaphore = xSemaphoreCreateBinary()) == NULL) {
        ESP_LOGE(BT_I2S_TAG, "%s, Semaphore create failed", __func__);
        return;
    }
    if ((s_ringbuf_i2s = xRingbufferCreate(RINGBUF_MAX_CAPACITY, RINGBUF_TYPE_BYTEBUF)) == NULL) {
        ESP_LOGE(BT_I2S_TAG, "%s, ringbuffer create failed", __func__);
        return;
    }
    xTaskCreate(bt_i2s_task_handler, "BtI2STask", 2048, NULL, configMAX_PRIORITIES - 3, &s_bt_i2s_task_handle);
}

void bt_i2s_task_shut_down(void)
{
    if (s_bt_i2s_task_handle) {
        vTaskDelete(s_bt_i2s_task_handle);
        s_bt_i2s_task_handle = NULL;
    }
    if (s_ringbuf_i2s) {
        vRingbufferDelete(s_ringbuf_i2s);
        s_ringbuf_i2s = NULL;
    }
    if (s_i2s_write_semaphore) {
        vSemaphoreDelete(s_i2s_write_semaphore);
        s_i2s_write_semaphore = NULL;
    }
}

size_t bt_i2s_async_write(const uint8_t *data, size_t size)
{
    size_t item_size = 0;
    BaseType_t done = pdFALSE;

    if (ringbuffer_mode == RINGBUFFER_MODE_DROPPING) {
        ESP_LOGW(BT_I2S_TAG, "ringbuffer is full, drop this packet!");
        vRingbufferGetInfo(s_ringbuf_i2s, NULL, NULL, NULL, NULL, &item_size);
        if (item_size <= RINGBUF_PREFETCH_CAPACITY) {
            ESP_LOGI(BT_I2S_TAG, "ringbuffer data decreased! mode changed: RINGBUFFER_MODE_PROCESSING");
            ringbuffer_mode = RINGBUFFER_MODE_PROCESSING;
        }
        return 0;
    }

    done = xRingbufferSend(s_ringbuf_i2s, (void *)data, size, (TickType_t)0);

    if (!done) {
        ESP_LOGW(BT_I2S_TAG, "ringbuffer overflowed, ready to decrease data! mode changed: RINGBUFFER_MODE_DROPPING");
        ringbuffer_mode = RINGBUFFER_MODE_DROPPING;
    }

    if (ringbuffer_mode == RINGBUFFER_MODE_PREFETCHING) {
        vRingbufferGetInfo(s_ringbuf_i2s, NULL, NULL, NULL, NULL, &item_size);
        if (item_size >= RINGBUF_PREFETCH_CAPACITY) {
            ESP_LOGI(BT_I2S_TAG, "ringbuffer data increased! mode changed: RINGBUFFER_MODE_PROCESSING");
            ringbuffer_mode = RINGBUFFER_MODE_PROCESSING;
            if (pdFALSE == xSemaphoreGive(s_i2s_write_semaphore)) {
                ESP_LOGE(BT_I2S_TAG, "semphore give failed");
            }
        }
    }

    return done ? size : 0;
}

esp_err_t bt_i2s_change_codec(int sample_rate, int ch_count){
    #ifdef CONFIG_EXAMPLE_A2DP_SINK_OUTPUT_INTERNAL_DAC
        dac_continuous_disable(tx_chan);
        dac_continuous_del_channels(tx_chan);
        dac_continuous_config_t cont_cfg = {
            .chan_mask = DAC_CHANNEL_MASK_ALL,
            .desc_num = 8,
            .buf_size = 2048,
            .freq_hz = sample_rate,
            .offset = 127,
            .clk_src = DAC_DIGI_CLK_SRC_DEFAULT,   // Using APLL as clock source to get a wider frequency range
            .chan_mode = (ch_count == 1) ? DAC_CHANNEL_MODE_SIMUL : DAC_CHANNEL_MODE_ALTER,
        };
        /* Allocate continuous channels */
        dac_continuous_new_channels(&cont_cfg, &tx_chan);
        /* Enable the continuous channels */
        esp_err_t ret = dac_continuous_enable(tx_chan);
        return ret;
    #else
        i2s_channel_disable(tx_chan);
        i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate);
        i2s_std_slot_config_t slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, ch_count);
        i2s_channel_reconfig_std_clock(tx_chan, &clk_cfg);
        i2s_channel_reconfig_std_slot(tx_chan, &slot_cfg);
        esp_err_t ret = i2s_channel_enable(tx_chan);
        return ret;
    #endif
}
