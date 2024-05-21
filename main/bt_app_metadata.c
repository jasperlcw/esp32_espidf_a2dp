#include "bt_app_metadata.h"
#include "esp_log.h"

void bt_metadata_rc(uint8_t attr_id, uint8_t *attr_text)
{
    switch(attr_id) {
    case 0x1: {
        ESP_LOGI(BT_METADATA_TAG, "Song: %s", attr_text);
        break;
    }
    case 0x2: {
        ESP_LOGI(BT_METADATA_TAG, "Artist: %s", attr_text);
        break;
    }
    case 0x4: {
        ESP_LOGI(BT_METADATA_TAG, "Album: %s", attr_text);
        break;
    }
    case 0x20: {
        ESP_LOGI(BT_METADATA_TAG, "Genre: %s", attr_text);
        break;
    }
    default: {
        ESP_LOGI(BT_METADATA_TAG, "Unhandled AVRC metadata: attribute id 0x%x, %s", attr_id, attr_text);
        break;
    }
    }
}
