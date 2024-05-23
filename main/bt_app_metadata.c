#include <string.h>
#include "bt_app_metadata.h"
#include "bt_app_uart.h"
#include "esp_avrc_api.h"
#include "esp_log.h"

unsigned char metadata_buf[512];

void bt_metadata_rc(uint8_t attr_id, uint8_t *attr_text)
{
    bool send_flag = false;
    switch(attr_id) {
    case ESP_AVRC_MD_ATTR_TITLE: {
        ESP_LOGI(BT_METADATA_TAG, "Title: %s", attr_text);
        send_flag = true;
        break;
    }
    case ESP_AVRC_MD_ATTR_ARTIST: {
        ESP_LOGI(BT_METADATA_TAG, "Artist: %s", attr_text);
        send_flag = true;
        break;
    }
    case ESP_AVRC_MD_ATTR_ALBUM: {
        ESP_LOGI(BT_METADATA_TAG, "Album: %s", attr_text);
        send_flag = true;
        break;
    }
    case ESP_AVRC_MD_ATTR_GENRE: {
        ESP_LOGI(BT_METADATA_TAG, "Genre: %s", attr_text);
        send_flag = true;
        break;
    }
    default: {
        ESP_LOGI(BT_METADATA_TAG, "Unhandled AVRC metadata: attribute id 0x%x, %s", attr_id, attr_text);
        break;
    }
    }

    if (send_flag) {
        sprintf(metadata_buf, "%x:%s", attr_id, attr_text);
        bt_uart_async_send(metadata_buf, strlen(metadata_buf));
    }
}
