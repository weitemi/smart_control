/*
 * @Author: your name
 * @Date: 2021-03-24 23:48:27
 * @LastEditTime: 2021-09-02 23:35:59
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\main\network\myble.c
 */


#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "myble.h"
#include "mywifi.h"

static const char *TAG = "myble.c";

static uint8_t adv_config_done = 0;

//wifi service的特征表
uint16_t wifi_handle_table[WIFISERV_IDX_NB];

typedef struct
{
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;

//ADV
static uint8_t raw_adv_data[] = {
    /* flags */
    0x02, 0x01, 0x06,
    /* tx power*/
    0x02, 0x0a, 0xeb,
    /* service uuid */
    0x03, 0x03, 0xFF, 0x00,
    /* device name */
    0x0f, 0x09, 'E', 'S', 'P', '3', '2', 'A', '1', 'S'};
static uint8_t raw_scan_rsp_data[] = {
    /* flags */
    0x02, 0x01, 0x06,
    /* tx power */
    0x02, 0x0a, 0xeb,
    /* service uuid */
    0x03, 0x03, 0xFF, 0x00};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

//GATT
struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

//内部函数声明
void wifiservice_handle(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static struct gatts_profile_inst wificonfig_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* 
  Service ：
    #define UUID_SERVCLASS_GAP_SERVER               0x1800  Generic Access
    #define UUID_SERVCLASS_GATT_SERVER              0x1801  Generic Attribute
*/

static const uint16_t UUID_SERVICE_WIFI = 0x6666;
static const uint16_t UUID_CHAR_SSID = 0x6601;
static const uint16_t UUID_CHAR_PSWD = 0x6602;
static const uint16_t UUID_CHAR_CONFIG = 0x6603;
static const uint16_t UUID_CHAR_WIFI_STATUS = 0x6604;

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t wifiservice_ccc[2] = {0x01, 0x00};
//char_value只是初始值，table里val的最大长度在h文件声明，是500
static const uint8_t char_value[4] = {0x11, 0x22, 0x33, 0x44};

/* Full Database Description - Used to add attributes into the database */
//service的特征表
static const esp_gatts_attr_db_t gatt_db[WIFISERV_IDX_NB] =
    {
        // 声明 WIFI Service
        [IDX_SVC] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(UUID_SERVICE_WIFI), (uint8_t *)&UUID_SERVICE_WIFI}},

        /* 声明SSID 的读写属性 */
        [IDX_CHAR_SSID] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

        /* 设置SSID的值 */
        [IDX_CHAR_VAL_SSID] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&UUID_CHAR_SSID, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

        /* 声明PSWD 的读写属性 */
        [IDX_CHAR_PSWD] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

        /* 设置PSWD的值 */
        [IDX_CHAR_VAL_PSWD] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&UUID_CHAR_PSWD, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

        /* 声明CONFIG 的读写属性 */
        [IDX_CHAR_CONF] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

        /* 设置CONFIG的值 */
        [IDX_CHAR_VAL_CONF] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&UUID_CHAR_CONFIG, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},

        /* 声明 WIFI STATUS 的读写通知属性 */
        [IDX_CHAR_STATUS] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

        /* 设置WIFI STATUS的值 */
        [IDX_CHAR_VAL_STATUS] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&UUID_CHAR_WIFI_STATUS, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, GATTS_DEMO_CHAR_VAL_LEN_MAX, sizeof(char_value), (uint8_t *)char_value}},
        // /* Client Characteristic Configuration Descriptor */
        [IDX_CHAR_CFG_STATUS] =
            {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(wifiservice_ccc), (uint8_t *)wifiservice_ccc}},

};

//gatt负责处理profile事件
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    //profile注册成功
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            wificonfig_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGI(TAG, "reg app failed, app_id %04x, status %d",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }
    do
    {
        int idx;
        //遍历profile表
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == wificonfig_profile_tab[idx].gatts_if)
            {
                if (wificonfig_profile_tab[idx].gatts_cb)
                {
                    //调用profile回调函数：gatts_profile_event_handler
                    wificonfig_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

/*
 GAP负责广播、连接
*/
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        //广播参数设置完成，开始广播
        adv_config_done &= (~ADV_CONFIG_FLAG);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        /* advertising start complete event to indicate advertising start successfully or failed */
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "advertising start failed");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGI(TAG, "Advertising stop failed");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

//将写入数据缓存起来
void prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;     //
    if (prepare_write_env->prepare_buf == NULL)
    {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL)
        {
            ESP_LOGI(TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    }
    else
    {
        if (param->write.offset > PREPARE_BUF_MAX_SIZE)
        {
            status = ESP_GATT_INVALID_OFFSET;
        }
        else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE)
        {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp)
    {
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL)
        {
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK)
            {
                ESP_LOGI(TAG, "Send response error");
            }
            free(gatt_rsp);
        }
        else
        {
            ESP_LOGI(TAG, "%s, malloc failed", __func__);
        }
    }
    if (status != ESP_GATT_OK)
    {
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf)
    {
        esp_log_buffer_hex(TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }
    else
    {
        ESP_LOGI(TAG, "ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf)
    {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

//处理profile
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    //app注册到gatt server
    case ESP_GATTS_REG_EVT:
    {
        ESP_LOGI(TAG, "GATT REGISTER");
        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
        if (set_dev_name_ret)
        {
            ESP_LOGI(TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
        esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
        if (raw_adv_ret)
        {
            ESP_LOGI(TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
        }
        //设置标志位，使GAP开始广播
        adv_config_done |= ADV_CONFIG_FLAG;
        esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
        if (raw_scan_ret)
        {
            ESP_LOGI(TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
        }
        //设置标志位，使GAP能响应扫描
        adv_config_done |= SCAN_RSP_CONFIG_FLAG;
        //创建一个service
        esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, WIFISERV_IDX_NB, SERVICE_UUID);
        if (create_attr_ret)
        {
            ESP_LOGI(TAG, "create attr table failed, error code = %x", create_attr_ret);
        }
    }
    break;
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(TAG, "GATT READ");
        break;
    case ESP_GATTS_WRITE_EVT:
        ESP_LOGI(TAG, "GATT WRITE");
        if (!param->write.is_prep)
        {
            // the data length of gattc write  must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
            ESP_LOGI(TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
            esp_log_buffer_hex(TAG, param->write.value, param->write.len);
            wifiservice_handle(gatts_if, param);

            printf("\n this \n");

            /* send response when param->write.need_rsp is true*/
            if (param->write.need_rsp)
            {
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
            }
        }
        else
        {
            /* handle prepare write */
            prepare_write_event_env(gatts_if, &prepare_write_env, param);
        }
        break;
    case ESP_GATTS_EXEC_WRITE_EVT:
        // the length of gattc prepare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
        ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        example_exec_write_event_env(&prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
        esp_log_buffer_hex(TAG, param->connect.remote_bda, 6);
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    //断开GATT连接，重新开始广播
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        break;
    //创建特征表完成，将wifi service的特征表复制到wifi_handle_table，开启wifi service
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    {
        if (param->add_attr_tab.status != ESP_GATT_OK)
        {
            ESP_LOGI(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
        }
        else if (param->add_attr_tab.num_handle != WIFISERV_IDX_NB)
        {
            ESP_LOGI(TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to WIFISERV_IDX_NB(%d)",
                     param->add_attr_tab.num_handle, WIFISERV_IDX_NB);
        }
        else
        {
            //ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
            //
            memcpy(wifi_handle_table, param->add_attr_tab.handles, sizeof(wifi_handle_table));
            esp_ble_gatts_start_service(wifi_handle_table[IDX_SVC]);
        }
        break;
    }
    case ESP_GATTS_STOP_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    case ESP_GATTS_UNREG_EVT:
    case ESP_GATTS_DELETE_EVT:
    default:
        break;
    }
}

//service处理
void wifiservice_handle(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    char ssid[32] = {0};
    char pswd[64] = {0};
    ESP_LOGI(TAG, "WIFI Service Table");

    if (wifi_handle_table[IDX_CHAR_VAL_SSID] == param->write.handle)
    {
        memset(ssid, 0, 32);
        strncpy(ssid, (char *)param->write.value, param->write.len);
    }
    if (wifi_handle_table[IDX_CHAR_VAL_PSWD] == param->write.handle)
    {
        memset(pswd, 0, 64);
        strncpy(pswd, (char *)param->write.value, param->write.len);
    }
    if (wifi_handle_table[IDX_CHAR_VAL_CONF] == param->write.handle)
    {
        if (strcmp("666", (char *)param->write.value)==0)
        {
            wifi_update(ssid, pswd);
        }
    }
    if (wifi_handle_table[IDX_CHAR_VAL_STATUS] == param->write.handle)
    {
        uint8_t notify_data[5] = {0, 0x01, 0x02};
        //本地修改value值好像不会触发这个回调函数，无法notify客户端
        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, wifi_handle_table[IDX_CHAR_VAL_STATUS],
                                    sizeof(notify_data), notify_data, false);
        printf("\n have set wifi status \n");
    }
}

void ble_init(void)
{
    esp_err_t ret;
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGI(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGI(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    ret = esp_bluedroid_enable();
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    ret = esp_ble_gatt_set_local_mtu(500);
}

/*
 * 打开ble
 * 初始化bluedroid蓝牙协议栈
 */
int ble_open()
{
    esp_err_t ret;
    esp_bluedroid_status_t status = esp_bluedroid_get_status();
    if (status == ESP_BLUEDROID_STATUS_ENABLED)
    {
        ESP_LOGI(TAG, "ble already open");
        return ESP_FAIL;
    }
    ret = esp_bluedroid_init();
    ret = esp_bluedroid_enable();
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    //注册profile
    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    ret = esp_ble_gatt_set_local_mtu(500);

    return ret;
}
/*
 * 关闭ble
 * 释放bluedroid任务内存，不会释放控制器的内存
 */
int ble_close()
{
    esp_err_t ret;
    esp_bluedroid_status_t status = esp_bluedroid_get_status();
    if (status == ESP_BLUEDROID_STATUS_UNINITIALIZED || status == ESP_BLUEDROID_STATUS_INITIALIZED)
    {
        ESP_LOGI(TAG, "ble already close");
        return ESP_FAIL;
    }
    ret = esp_ble_gatts_app_unregister(wificonfig_profile_tab[PROFILE_APP_IDX].gatts_if);
    ret = esp_ble_gap_stop_advertising();
    ret = esp_bluedroid_disable();
    ret = esp_bluedroid_deinit();

    return ret;
}
/*
void set_wifi_status(uint8_t sta)
{

    esp_ble_gatts_set_attr_value(wifi_handle_table[IDX_CHAR_VAL_STATUS], 1, (const uint8_t *)&sta);
}
*/
