#ifndef _MYNVS_H
#define _MYNVS_H

#include "nvs_flash.h"
#include "nvs.h"
#include "irTask.h"
#include "esp_wifi.h"

#define IR_STORAGE_NAMESPACE "ir_data"
#define WIFI_STORAGE_NAMESPACE "wifi_data"
esp_err_t nvs_save_items(rmt_item32_t *item, size_t items_size, const char *name);
rmt_item32_t *nvs_get_items(size_t *item_size, const char *key);
esp_err_t nvs_delete_items(const char *key);
//esp_err_t nvs_read_wifi_config();
//esp_err_t nvs_save_wifi_config(const wifi_config_t wifi_config);

#endif