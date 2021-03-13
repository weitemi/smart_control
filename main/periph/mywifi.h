/*
 * @Author: your name
 * @Date: 2021-03-06 09:50:47
 * @LastEditTime: 2021-03-06 11:34:53
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_smartcontrol\main\periph\mywifi.h
 */
#ifndef _MYWIFI_H
#define _MYWIFI_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_wifi.h"





wifi_config_t wifi_config;
void wifi_init_sta(void);
void wifi_update();
int get_wifi_status();

#endif
