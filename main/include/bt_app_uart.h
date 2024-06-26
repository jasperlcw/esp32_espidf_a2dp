#ifndef __BT_APP_UART_H__
#define __BT_APP_UART_H__

#include <stddef.h>
#include "esp_err.h"

/* log tags */
#define BT_UART_TAG       "BT_UART"

/**
 * @brief Install/initialize the UART driver
 * 
 */
void bt_uart_driver_install(void);

/**
 * @brief Uninstall/clean up the UART driver
 * 
 */
void bt_uart_driver_uninstall(void);

/**
 * @brief Starts the task for UART
 * 
 */
esp_err_t bt_uart_task_start(void);

/**
 * @brief Stops the task for UART
 * 
 */
void bt_uart_task_stop(void);

/**
 * @brief Sends data over the UART bus. 
 *        Must be called after bt_uart_task_start for data to be sent.
 * 
 * @param data  bytes to send over the UART bus
 * @param len   number of bytes to send over
 * 
 * @return Number of bytes queued to be sent
 */
size_t bt_uart_async_send(const char *data, size_t len);

#endif