/*
 * @Author: your name
 * @Date: 2021-06-04 23:21:35
 * @LastEditTime: 2021-07-31 10:54:41
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\main\network\mymqtt.c
 */
#include "mymqtt.h"

static const char *TAG = "mqtt client";

//MQTT提供的接口
static EventGroupHandle_t mqtt_api;


const int PUBLISH_COMMAND_RESPONSE = BIT0;  //MQTT发送响应
const int PUBLISH_AC_STATUS = BIT1;  //mqtt发送空调状态
const int PUBLISH_GET_RESPONSE = BIT2;  //MQTT发送响应

static char mqtt_recv_buff[MQTT_RECV_BUFF_LEN] = {0};   //mqtt数据接收buff
static char topic_response[MQTT_RESPONSE_TOPIC_LEN] = {0};  //响应消息的主题

void mqtt_User_task(void *agr);
void mqtt_recv_handler(enum topic_type topic);
void mqtt_response(enum topic_type type, char *request_id);


const char *str_get_real_temp = "get_real_temp";
const char *str_ac_control = "ac_control";
static const char *device_id = "60b9c9383744a602a5cb9bf3_smart_control_01";

/*
 * @brief mqtt事件回调函数
 *
 */
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id = -1;
    static int total_recv = 0, request_id_len = 0;
    char *t;
    char request_id[48] = {0}; //存放request_id
    char *recv_buff = (char *)event->user_context;
    enum topic_type type = TOPIC_MAX;
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        //连接成功，发布一个无负载的消息
        msg_id = esp_mqtt_client_publish(client, TOPIC_REPORT, NULL, 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        //订阅主题
        //msg_id = esp_mqtt_client_subscribe(client, TOPIC_TEMP, 0);

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
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        
        //根据topic判断控制台消息类型
        t = strstr(event->topic, "request_id");
        if (t)
        {
            strncpy(request_id, t, 47);
            request_id[47] = '\0';
        }
        ESP_LOGI(TAG, "%s", request_id);

        if (strstr(event->topic, "/commands/"))
        {
            //下发设备命令：
            type = MQTT_COMMAND;
        }
        else if (strstr(event->topic, "/properties/get/"))
        {
            //查询设备属性：
            type = MQTT_CHECK;
        }
        //判断接收缓存够不够
        if (event->total_data_len > 1024)
        {
            ESP_LOGI(TAG, "recv buffer too small");
        }
        else
        {
            //将数据保存到本地缓存
            strncpy(recv_buff + event->current_data_offset, event->data, event->data_len);
            total_recv += event->data_len;
        }
        if (total_recv == event->total_data_len)
        {
            total_recv = 0; //接收完成
            //数据处理
            mqtt_recv_handler(type);

            //回复broker，每次接收到broker下发的消息，都需要回复，broker才能得知设备在线情况
            mqtt_response(type, request_id);
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
 * @brief mqtt接收数据处理.
 * topic_type：由回调函数解析出来的topic类型
 * mqtt数据缓存在mqtt_recv_buff[]中。
 */
void mqtt_recv_handler(enum topic_type topic)
{
    //broker下发的控制命令
    if (topic == MQTT_COMMAND)
    {
        //控制台命令消息类型如下
        /* {"paras":{"ac_power":1,"ac_temp":27,"ac_wind_speed":2,"ac_mode":0},"service_id":"ac_control","command_name":"ac_control"}*/
        cJSON *root = cJSON_Parse(mqtt_recv_buff);
        if (root != NULL)
        {
            cJSON *paras = cJSON_GetObjectItem(root, "paras");
            cJSON *services_id = cJSON_GetObjectItem(root, "service_id");
            char *str = cJSON_GetStringValue(services_id);
            ESP_LOGI(TAG, "service:%s", str);

            //service 正确
            if (strncmp(str_ac_control, str, strlen(str_ac_control)) == 0)
            {
                cJSON *command_name = cJSON_GetObjectItem(root, "command_name");
                str = cJSON_GetStringValue(command_name);
                ESP_LOGI(TAG, "command_name:%s", str);
                //ac_control命令
                if (strncmp(str_ac_control, str, strlen(str_ac_control)) == 0)
                {
                    cJSON *power = cJSON_GetObjectItem(paras, "ac_power");

                    cJSON *temp = cJSON_GetObjectItem(paras, "ac_temp");

                    cJSON *wind_speed = cJSON_GetObjectItem(paras, "ac_wind_speed");

                    cJSON *mode = cJSON_GetObjectItem(paras, "ac_mode");

                    //调用红外模块的接口，设置空调
                    ac_status_config(power->valueint, temp->valueint, wind_speed->valueint, mode->valueint);
                    
                }

            }

            cJSON_Delete(root);
        }
    }
    //控制台查询属性命令 其数据不需要处理
    else if (topic == MQTT_CHECK)
    {
        /* {"service_id":"ac_control"} */
        ESP_LOGI(TAG, "%s", mqtt_recv_buff);
    }
    //清空接收buff，准备接收下次数据
    memset(mqtt_recv_buff, 0, MQTT_RECV_BUFF_LEN);
}
/*
 * @breif 组装 设备端响应 主题，并同步用户线程
 * 每次broker下发消息，设备都需要给指定主题响应
 * 该API通过事件组标志与mqtt任务通信，不会长时间阻塞
 */
void mqtt_response(enum topic_type type, char *request_id)
{
    //清空响应主题
    memset(topic_response, 0, MQTT_RESPONSE_TOPIC_LEN);
    //根据broker下发的不同消息，组装topic
    switch (type)
    {
    case MQTT_COMMAND:
        sprintf(topic_response, "$oc/devices/%s/sys/commands/response/%s", device_id, request_id);
        //调用响应接口
        xEventGroupSetBits(mqtt_api, PUBLISH_COMMAND_RESPONSE);  
        break;
    case MQTT_CHECK:
        sprintf(topic_response, "$oc/devices/%s/sys/properties/get/response/%s", device_id, request_id);
        xEventGroupSetBits(mqtt_api, PUBLISH_GET_RESPONSE);
        break;
    default:
        break;
    }

    ESP_LOGI(TAG, "response topic:%s", topic_response);
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
        .user_context = (void *)mqtt_recv_buff,

    };
    //wifi disconnect return
    if (!get_wifi_status())
    {
        return;
    }
    //提供其他task的调用接口
    mqtt_api = xEventGroupCreate();
    xEventGroupClearBits(mqtt_api, PUBLISH_COMMAND_RESPONSE | PUBLISH_AC_STATUS | PUBLISH_GET_RESPONSE);

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    //注册回调函数
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);

    xTaskCreate(mqtt_User_task, "mqtttask", 3 * 1024, client, 5, NULL);
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
        //等待用户调用接口
        int bit = xEventGroupWaitBits(mqtt_api, PUBLISH_COMMAND_RESPONSE | PUBLISH_AC_STATUS | PUBLISH_GET_RESPONSE, pdTRUE, pdFAIL, 100 / portTICK_RATE_MS);
        xEventGroupClearBits(mqtt_api, PUBLISH_AC_STATUS); //还是要清除一下
        
        //broker要求上报设备属性 或 设备主动上报属性
        if ((bit & PUBLISH_AC_STATUS) || (bit & PUBLISH_GET_RESPONSE))
        {
            /*调用红外模块的接口，构造json字符串*/
            cJSON *temp = cJSON_CreateNumber(ac_get_temp());
            cJSON *power = cJSON_CreateNumber(ac_get_power());
            cJSON *mode = cJSON_CreateNumber(ac_get_mode());
            cJSON *wind_speed = cJSON_CreateNumber(ac_get_wind_speed());
            cJSON *real_temp = cJSON_CreateNumber(ds18b20_get_data());

            cJSON *properties = cJSON_CreateObject();
            cJSON_AddItemToObject(properties, "ac_temp", temp);
            cJSON_AddItemToObject(properties, "ac_power", power);
            cJSON_AddItemToObject(properties, "ac_mode", mode);
            cJSON_AddItemToObject(properties, "ac_wind_speed", wind_speed);
            cJSON_AddItemToObject(properties, "real_temp", real_temp);

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

            //给broker不同主题响应设备属性
            if (bit & PUBLISH_GET_RESPONSE)
            {
                msg_id = esp_mqtt_client_publish(client, topic_response, res, strlen(res), 1, 0);
            }
            else
            {
                msg_id = esp_mqtt_client_publish(client, TOPIC_REPORT, res, strlen(res), 1, 0);
            }

            ESP_LOGI(TAG, "mqtt publish ac_status msg_id=%d", msg_id);

            cJSON_Delete(root);

           
        }
        //响应 broker 的设备控制命令
        if (bit & PUBLISH_COMMAND_RESPONSE)
        {
            //发送 成功响应
            cJSON *result = cJSON_CreateString("success");
            cJSON *paras = cJSON_CreateObject();
            cJSON_AddItemToObject(paras, "result", result);
            cJSON *root = cJSON_CreateObject();
            cJSON_AddItemToObject(root, "paras", paras);
            char *res = cJSON_Print(root);

            msg_id = esp_mqtt_client_publish(client, topic_response, res, strlen(res), 1, 0);
            ESP_LOGI(TAG, "mqtt response borker msg_id=%d", msg_id);

            cJSON_Delete(root);
        }
    }
}