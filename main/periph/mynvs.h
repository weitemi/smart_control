/*
 * @Author: your name
 * @Date: 2021-03-06 11:04:17
 * @LastEditTime: 2021-03-16 14:32:39
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\main\periph\mynvs.h
 */
#ifndef _MYNVS_H
#define _MYNVS_H

#include "nvs_flash.h"
#include "nvs.h"
#include "irTask.h"
#include "esp_wifi.h"

#define IR_STORAGE_NAMESPACE "ir_data"
#define WIFI_STORAGE_NAMESPACE "wifi_data"
#define AC_HANDLE_STORAGE_NAMESPACE "ac_handle"
struct AC_Control *nvs_get_ac_handle(size_t *len, const char *key);
esp_err_t nvs_save_ac_handle(struct AC_Control* ac_handle,const char *key,size_t ac_size);
esp_err_t nvs_save_items(rmt_item32_t *item, size_t items_size, const char *name);
rmt_item32_t *nvs_get_items(size_t *item_size, const char *key);
esp_err_t nvs_delete_items(const char *key);


#endif