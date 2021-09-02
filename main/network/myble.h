/*
 * @Author: your name
 * @Date: 2021-03-24 23:48:44
 * @LastEditTime: 2021-09-02 22:55:17
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\main\network\myble.h
 */
#ifndef _MY_BLE_H
#define _MY_BLE_H

#include "main.h"



/* wifi service Attributes State Machine */
enum
{
    IDX_SVC,
    IDX_CHAR_SSID,
    IDX_CHAR_VAL_SSID,
    IDX_CHAR_PSWD,
    IDX_CHAR_VAL_PSWD,
    IDX_CHAR_CONF,
    IDX_CHAR_VAL_CONF,
    IDX_CHAR_STATUS,
    IDX_CHAR_VAL_STATUS,
    IDX_CHAR_CFG_STATUS,
    WIFISERV_IDX_NB,
};

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SAMPLE_DEVICE_NAME          "SMART_CONTROL"
//服务的uuid
#define SERVICE_UUID                 0

/* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
*  the data length must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX. 
*/
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 100
#define PREPARE_BUF_MAX_SIZE        512
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

//todo 使操作线程安全
void ble_init(void);
int ble_close();
int ble_open();

#endif  // _MY_BLE_H