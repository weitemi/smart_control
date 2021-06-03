/*
 * @Author: your name
 * @Date: 2021-03-06 09:50:47
 * @LastEditTime: 2021-06-03 23:58:31
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


void wifi_init_sta(void);
int wifi_update(const char *ssid,const char *password);
int get_wifi_status();

#endif
