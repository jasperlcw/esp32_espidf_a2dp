#include "bt_app_metadata.h"
#include "esp_avrc_api.h"
#include "esp_log.h"

void bt_metadata_rc(uint8_t attr_id, uint8_t *attr_text)
{
    switch(attr_id) {
        /* Fall through, send data over to another device over UART to process */
    case ESP_AVRC_MD_ATTR_TITLE: {}
    case ESP_AVRC_MD_ATTR_ARTIST: {}
    case ESP_AVRC_MD_ATTR_ALBUM: {}
    case ESP_AVRC_MD_ATTR_GENRE: {
        // Send AVRC metadata over UART to a target device
        break;
    }
    default: {
        ESP_LOGI(BT_METADATA_TAG, "Unhandled AVRC metadata: attribute id 0x%x, %s", attr_id, attr_text);
        break;
    }
    } 
}
