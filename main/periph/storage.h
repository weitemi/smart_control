/*
 * @Author: your name
 * @Date: 2021-06-03 19:11:52
 * @LastEditTime: 2021-06-04 08:36:23
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\main\periph\storage.h
 */
#ifndef _STORAGE_H
#define _STORAGE_H

#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "ir.h"

#define AC_DEFAULT "ac-code"
#define IR_STORAGE_NAMESPACE "ir_data"
#define WIFI_STORAGE_NAMESPACE "wifi_data"
#define AC_CODE_NAMESPACE "ac_code"

//nvs 
uint8_t *nvs_get_ac_lib(const char *key);
esp_err_t nvs_save_ac_code(uint8_t code, const char *key);
esp_err_t nvs_save_items(rmt_item32_t *item, size_t items_size, const char *name);
rmt_item32_t *nvs_get_items(size_t *item_size, const char *key);
esp_err_t nvs_delete_items(const char *key);

//spiffs
int storage_init(); //存储系统，文件系统初始化

#endif