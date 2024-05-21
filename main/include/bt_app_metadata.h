#ifndef __BT_APP_METADATA_H__
#define __BT_APP_METADATA_H__

#include <stdint.h>

/* log tags */
#define BT_METADATA_TAG       "BT_METADATA"

void bt_metadata_rc(uint8_t attr_id, uint8_t *attr_text);

#endif