#ifndef _BT_APP_I2S_H_
#define _BT_APP_I2S_H_

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

/* log tags */
#define BT_I2S_TAG       "BT_I2S"

/**
 * @brief  Starts up the I2S task
 */
void bt_i2s_task_start_up(void);

/**
 * @brief  Shuts down the I2S task
 */
void bt_i2s_task_shut_down(void);

/**
 * @brief Install/initialize the i2s driver 
 */
void bt_i2s_driver_install(void);

/**
 * @brief Uninstall/clean up the i2s driver 
 */
void bt_i2s_driver_uninstall(void);

/**
 * @brief  Queues data to write to the I2S bus asynchronously.
 *
 * @param [in] data  pointer to data stream
 * @param [in] size  data length in byte
 *
 * @return Number of bytes buffered for output. Zero bytes indicate failure.
 */
size_t bt_i2s_async_write(const uint8_t *data, size_t size);


/**
 * @brief Reconfigure the i2s driver to output the specified codec
 * 
 * @param sample_rate sample rate to output
 * @param ch_count channel count to output
 */
esp_err_t bt_i2s_change_codec(int sample_rate, int ch_count);

#endif