/*
 * @Author: your name
 * @Date: 2021-06-04 23:21:35
 * @LastEditTime: 2021-06-05 13:14:53
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\main\network\mymqtt.c
 */
#include "mymqtt.h"

static const char *TAG = "mqtt client";

static EventGroupHandle_t mqtt_api;
const int PUBLISH_TEMP = BIT0;
const int PUBLISH_AC_STATUS = BIT1;

static char mqtt_recv_buff[MQTT_RECV_BUFF_LEN] = {0};

void mqtt_User_task(void *agr);
void mqtt_recv_handler(enum topic_type topic);
/*
 * @brief mqtt事件回调函数
 *
 */
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id = -1;
    static int total_recv = 0;
    char *recv_buff =(char *) event->user_context;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        //连接成功，发布一个无负载的消息
        msg_id = esp_mqtt_client_publish(client, TOPIC_REPORT, "data", 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        //订阅主题
        msg_id = esp_mqtt_client_subscribe(client, TOPIC_TEMP, 0);

        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);

        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        //MQTT接收到消息
        if (event->topic_len > 20)
        {
            ESP_LOGI(TAG, "recv from console,total=%d,data_offset=%d,data_len=%d", event->total_data_len, event->current_data_offset, event->data_len);
            if (event->total_data_len > 1024)
            {
                ESP_LOGI(TAG, "recv buffer too small");
            }
            else
            {
                strncpy(recv_buff+event->current_data_offset, event->data, event->data_len);
                total_recv += event->data_len;
            }
            if (total_recv == event->total_data_len)
            {
                total_recv = 0; //接收完成
                //数据处理
                mqtt_recv_handler(CONSOLE_ORDER);
            }
        }
        else
        {
            //todo 用户的topic
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{

    mqtt_event_handler_cb(event_data);
}

/*
 * @brief mqtt接收数据处理
 */
void mqtt_recv_handler(enum topic_type topic)
{
    //来自华为云控制台的控制消息
    if (topic == CONSOLE_ORDER)
    {
        //目前两种消息类型如下
        /* {"paras":{"ac_power":1,"ac_temp":27,"ac_wind_speed":2,"ac_mode":0},"service_id":"ac_control","command_name":"ac_control"}*/
        /* {"paras":{"temp":1},"service_id":"get_temp","command_name":"get_temp"} */
        //ESP_LOGI(TAG, "%s", mqtt_recv_buff);
        cJSON *root = cJSON_Parse(mqtt_recv_buff);
    
        if (root != NULL)
        {
            cJSON *paras = cJSON_GetObjectItem(root, "paras");
            cJSON *services_id = cJSON_GetObjectItem(root, "service_id");
            char *id = cJSON_GetStringValue(services_id);
            ESP_LOGI(TAG, "service : %s", id);
            if (strncmp(GET_TEMP_SERVICE_ID, id, GET_TEMP_SERVICE_ID_LEN) == 0)
            {
                cJSON *element = cJSON_GetObjectItem(paras, "temp");
                //ESP_LOGI(TAG, "get temp:%d", element->valueint);
                mqtt_publish_temp();

            }
            else if (strncmp(AC_CONTROL_SERVICE_ID, id, AC_CONTROL_SERVICE_ID_LEN) == 0)
            {
                cJSON *power = cJSON_GetObjectItem(paras, "ac_power");

                cJSON *temp = cJSON_GetObjectItem(paras, "ac_temp");
   
                cJSON *wind_speed = cJSON_GetObjectItem(paras, "ac_wind_speed");
       
                cJSON *mode = cJSON_GetObjectItem(paras, "ac_mode");

                //设置空调
                ac_status_config(power->valueint, temp->valueint, wind_speed->valueint, mode->valueint);
            }

            cJSON_Delete(root);
        }
        //清空buff，准备接收下次数据
        memset(mqtt_recv_buff, 0, MQTT_RECV_BUFF_LEN);
        
    }
}
/*
 * @brief mqtt初始化 开启mqtt客户端，连接到华为云
 *
 */
void mqtt_init()
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    esp_mqtt_client_config_t mqtt_cfg = {
        .host = HOST,
        .port = PORT,
        .password = PSW,
        .client_id = CLIENT_ID,
        .username = USER_NAME,
        .user_context=(void *)mqtt_recv_buff,

    };
    //wifi disconnect return
    if (!get_wifi_status())
    {
        return;
    }
    //提供其他task的调用接口
    mqtt_api = xEventGroupCreate();
    xEventGroupClearBits(mqtt_api, PUBLISH_TEMP | PUBLISH_AC_STATUS);

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    //注册回调函数
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);

    xTaskCreate(mqtt_User_task, "mqtttask", 3 * 1024, client, 5, NULL);
}

/*
 * @breif mqtt客户端发布 温度消息
 * 该API通过事件组标志与mqtt任务通信，不会长时间阻塞
 */
void mqtt_publish_temp()
{
    xEventGroupSetBits(mqtt_api, PUBLISH_TEMP);
}

/*
 * @breif mqtt客户端发布 空调消息
 * 该API通过事件组标志与mqtt任务通信，不会长时间阻塞
 */
void mqtt_publish_ac_status()
{
    xEventGroupSetBits(mqtt_api, PUBLISH_AC_STATUS);
}

/*
 * @brief 用户mqtt任务，不同于系统的内置mqtt任务，对外提供API
 */
void mqtt_User_task(void *agr)
{
    int msg_id = -1;
    char *res;
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)agr;
    while (1)
    {
        int bit = xEventGroupWaitBits(mqtt_api, PUBLISH_TEMP, pdTRUE, pdFAIL, 100 / portTICK_RATE_MS);
        xEventGroupClearBits(mqtt_api, PUBLISH_AC_STATUS | PUBLISH_TEMP); //还是要清除一下
        
        if (bit & PUBLISH_TEMP)
        {
            cJSON *temp = cJSON_CreateNumber(ds18b20_get_data());

            cJSON *properties = cJSON_CreateObject();

            cJSON *service_id = cJSON_CreateString("get_temp");

            cJSON_AddItemToObject(properties, "temp", temp);

            cJSON *json = cJSON_CreateObject();

            cJSON_AddItemToObject(json, "service_id", service_id);
            cJSON_AddItemToObject(json, "properties", properties);

            cJSON *services = cJSON_CreateArray();
            cJSON_AddItemToArray(services, json);

            cJSON *root = cJSON_CreateObject();
            cJSON_AddItemToObject(root, "services", services);
            res = cJSON_Print(root);

            msg_id = esp_mqtt_client_publish(client, TOPIC_REPORT, res, strlen(res), 1, 0);

            cJSON_Delete(root);

            free(res);
        }
        if (bit & PUBLISH_AC_STATUS)
        {
            //创建属性
            cJSON *temp = cJSON_CreateNumber(ac_get_temp());
            cJSON *power = cJSON_CreateNumber(ac_get_power());
            cJSON *mode = cJSON_CreateNumber(ac_get_mode());
            cJSON *wind_speed = cJSON_CreateNumber(ac_get_wind_speed());

            cJSON *properties = cJSON_CreateObject();
            cJSON_AddItemToObject(properties, "ac_temp", temp);
            cJSON_AddItemToObject(properties, "ac_power", power);
            cJSON_AddItemToObject(properties, "ac_mode", mode);
            cJSON_AddItemToObject(properties, "ac_wind_speed", wind_speed);

            //确定服务id
            cJSON *service_id = cJSON_CreateString("ac_control");

            //固定格式
            cJSON *json = cJSON_CreateObject();

            cJSON_AddItemToObject(json, "service_id", service_id);
            cJSON_AddItemToObject(json, "properties", properties);

            cJSON *services = cJSON_CreateArray();
            cJSON_AddItemToArray(services, json);

            cJSON *root = cJSON_CreateObject();
            cJSON_AddItemToObject(root, "services", services);
            res = cJSON_Print(root);

            msg_id = esp_mqtt_client_publish(client, TOPIC_REPORT, res, strlen(res), 1, 0);

            cJSON_Delete(root);

            free(res);
        }
    }
}