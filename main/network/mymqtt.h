/*
 * @Author: your name
 * @Date: 2021-06-04 23:21:25
 * @LastEditTime: 2021-07-31 10:58:50
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\main\network\mymqtt.h
 */
#ifndef _MYMQTT_H
#define _MYMQTT_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_system.h"
#include "esp_event.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mywifi.h"
#include "cJSON.h"
#include "esp_event.h"
#include "myds18b20.h"
#include "ir.h"

//#define TOPIC_TEMP "$oc/devices/60b9c9383744a602a5cb9bf3_smart_control_01/user/temp"
//#define TOPIC_AC "$oc/devices/60b9c9383744a602a5cb9bf3_smart_control_01/user/ac_status"
//#define TOPIC_PROPERTIES_SET "$oc/devices/60b9c9383744a602a5cb9bf3_smart_control_01/sys/properties/set/#"
#define TOPIC_REPORT "$oc/devices/60b9c9383744a602a5cb9bf3_smart_control_01/sys/properties/report"

#define PSW "f0cd68624a1796c7b752ffd73a2df073606402e580d37c228c073691aab76c1a"
#define CLIENT_ID "60b9c9383744a602a5cb9bf3_smart_control_01_0_0_2021060407"
#define USER_NAME "60b9c9383744a602a5cb9bf3_smart_control_01"

#define PORT 1883
#define HOST "121.36.42.100"

#define MQTT_RECV_BUFF_LEN 1024
#define MQTT_RESPONSE_TOPIC_LEN 200


enum topic_type{
    MQTT_COMMAND,
    MQTT_CHECK,

    TOPIC_MAX
};

void mqtt_init();
void mqtt_publish_ac_status();

#endif
