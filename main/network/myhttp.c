#include "myhttp.h"
#include "mywifi.h"
#include "esp_audio.h"
#include "player.h"
#include "clock.h"

static const char *TAG = "myhttp";
static const char *wea_code = "200";

char baidu_access_token[80] = "24.fa404e41d09ac1b28eecd91edbf6d238.2592000.1617034369.282335-23021308";

#define GET_TOKEN_URL "https://openapi.baidu.com/oauth/2.0/token?grant_type=client_credentials&client_id=G86ZG9H52Mi1Ngf0uByK4IbA&client_secret=WkelePkHg8zozxwztB2gvGq1kF9AOUgL"
#define WEATHER_URL "https://devapi.qweather.com/v7/weather/3d?location=101280604&key=5dc3077a2aab447ca6bcbd62e22cc068&gzip=n"
#define TIME_URL "https://api.uukit.com/time"

#define MX_HTTP_BUFF 2048
EXT_RAM_ATTR static char http_data[MX_HTTP_BUFF] = {0}; //! 需要定义为全局静态变量，否则会导致任务的堆栈溢出

const int GET_TIME_BIT = BIT0;
const int GET_WEATHER_BIT = BIT1;
const int UPDATE_TOKEN_BIT = BIT4;


static EventGroupHandle_t http_api_evengroup; //api任务事件组

QueueHandle_t res_queue;    //http处理结果

/*
 * http传输过程回调函数
 * evt：回调事件
 * evt->user_data：指向缓存http数据的内存
 */
esp_err_t http_event_handle(esp_http_client_event_t *evt)
{
    static int index = 0;
    //判断事件类型
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        printf("http_err\r\n");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        //printf("http connected \r\n");
        break;
    case HTTP_EVENT_HEADER_SENT:
        //printf( "http send data\r\n");
        break;
    case HTTP_EVENT_ON_HEADER:
        //printf( "http fetch header\r\n");

        break;
    case HTTP_EVENT_ON_DATA:
        //正在接收数据
        //printf( "http receive data,len=%d\r\n", evt->data_len);
        //printf("---http data---\r\n%.*s \r\n ---data end--- \r\n", evt->data_len, (char *)evt->data);

        memcpy(evt->user_data + index, evt->data, evt->data_len); //将http数据复制到指定内存
        index += evt->data_len;                                   //下标增加

        break;
    case HTTP_EVENT_ON_FINISH:
        //printf("http receive finish\r\n");

        //printf("---http receive buff---\r\n %.*s \r\n ---buff end--- \r\n", index, (char *)evt->user_data);
        index = 0; //数据传输完成

        break;
    case HTTP_EVENT_DISCONNECTED:
        //printf("http disconnected\r\n");

        break;
    default:
        break;
    }
    return ESP_OK;
}

/*
 * 更新百度token
 * 返回：ESP_OK
 */
int update_access_token()
{
    int msg = 0;
    xEventGroupSetBits(http_api_evengroup, UPDATE_TOKEN_BIT); //设置http_api_evengroup，同步http_api_task
    xQueueReceive(res_queue, &msg, portMAX_DELAY);
    return msg;
}
/*
 * 获取天气数据字符串 
 * day:1,今天 2,明天,3后天
 * 返回：天气数据字符串指针
 */
static char str_weather[3][100] = {0}; //存放天气字符串
char *get_Weather_String(int day)
{
    int msg = 0;
    //检查day
    if (day < 1 || day > 3)
    {
        return NULL;
    }
    xEventGroupSetBits(http_api_evengroup, GET_WEATHER_BIT); //设置GET_WEATHER_BIT，同步http_api_task

    //若已获取天气数据，则GET_WEATHER_OK_BIT被置位，返回天气文本的地址
    xQueueReceive(res_queue, &msg, portMAX_DELAY);
    if (msg==ESP_OK)
    {
        return &str_weather[day-1][0];
    }

    return NULL;
}

static char str_time[25] = {0}; //时间文本

/*
 * 获取网络时间，阻塞直到获取到时间
 * 返回 ：时间字符串
 */
char *get_Time_String()
{
    int msg = 0;
    xEventGroupSetBits(http_api_evengroup, GET_TIME_BIT); //设置GET_TIME_BIT，同步http_api_task

    //等待http_api_task完成获取
    xQueueReceive(res_queue, &msg, portMAX_DELAY);
    //成功则返回时间数据的指针
    if (msg==ESP_OK)
    {
        return str_time;
    }
    return NULL;
}

/*
 * 通过http协议调用api
 * 通过事件组的方式同步任务运行
 */
void http_api_task(void *arg)
{
    esp_http_client_handle_t client;

    //初始化http客户端
    esp_http_client_config_t config = {
        .event_handler = http_event_handle, //http回调函数
        .user_data = (void *)http_data,     //http数据缓存
    };
    int res = ESP_OK;
    while (1)
    {
        //等待用户调用api（获取时间，获取天气，获取baidutoken）
        EventBits_t bit = xEventGroupWaitBits(http_api_evengroup, GET_TIME_BIT | GET_WEATHER_BIT | UPDATE_TOKEN_BIT, pdTRUE, pdFAIL, portMAX_DELAY);
        //ESP_LOGI(TAG, "bit = %d", bit);
        //判断网络可用？
        if (!get_wifi_status())
        {
            bit = 0;
            xEventGroupClearBits(http_api_evengroup, GET_TIME_BIT | GET_WEATHER_BIT | UPDATE_TOKEN_BIT); //无法执行，清除标志位
            ESP_LOGI(TAG, "error WIFI does not connected");
            res = ESP_FAIL;
            
        }
        //网络已经连接，处理不同的api
        if (bit & GET_TIME_BIT)
        {
            //获取网络时间
            //ESP_LOGI(TAG, "GET_TIME_BIT");

            memset(http_data, 0, MX_HTTP_BUFF); //清空http缓存区
            config.url = TIME_URL;              //设置url，请求方式
            config.method = HTTP_METHOD_GET;
            ESP_LOGI(TAG,"start connect to url = %s\r\n", config.url);
            client = esp_http_client_init(&config);
            esp_http_client_perform(client); //发起http连接
            esp_http_client_close(client);
            esp_http_client_cleanup(client);

            cJSON *root = cJSON_Parse(http_data); //解析返回的时间json数据
            if (root != NULL)
            {
                cJSON *time = cJSON_GetObjectItem(root, "data");
                time = cJSON_GetObjectItem(time, "gmt");
                char *t = cJSON_GetStringValue(time);
                if (t == NULL)
                {
                    ESP_LOGI(TAG, "time error");
                    res = ESP_FAIL;
                    
                }
                else
                {

                    ESP_LOGI(TAG, "time = %s", t);

                    strncpy(str_time, t, 25); //将字符串复制到str_time

                    cJSON_Delete(root);
                    res = ESP_OK;
                }
            }
        }
        if (bit & GET_WEATHER_BIT)
        {
            //获取天气
            //ESP_LOGI(TAG, "GET_WEATHER_BIT");

            memset(http_data, 0, MX_HTTP_BUFF);
            config.url = WEATHER_URL;
            config.method = HTTP_METHOD_GET;

            ESP_LOGI(TAG,"start connect to url = %s\r\n", config.url);
            client = esp_http_client_init(&config);
            esp_http_client_perform(client);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);

            cJSON *root = cJSON_Parse(http_data);
            cJSON *code = cJSON_GetObjectItem(root, "code");
            if (code == NULL)
            {
                cJSON_Delete(root);
                res = ESP_FAIL;
            }
            //检验代码是否是正确
            char *code_str = cJSON_GetStringValue(code);
            if (strcmp(code_str, wea_code))
            {
                ESP_LOGI(TAG, "get weather fail code = %s", code_str);
                cJSON_Delete(root);
                res = ESP_FAIL;
            }
            //到这获取成功，读取实时数据
            cJSON *daily = cJSON_GetObjectItem(root, "daily");
            //获取今天，明天，后天的数据
            for (int i = 0; i < 3; i++)
            {

                cJSON *today = cJSON_GetArrayItem(daily, i);
                cJSON *item = cJSON_GetObjectItem(today, "fxDate");
                char *time = cJSON_GetStringValue(item);
                item = cJSON_GetObjectItem(today, "tempMax");
                char *tempmax = cJSON_GetStringValue(item);
                item = cJSON_GetObjectItem(today, "tempMin");
                char *tempmin = cJSON_GetStringValue(item);
                item = cJSON_GetObjectItem(today, "windScaleDay");
                char *windscale = cJSON_GetStringValue(item);
                item = cJSON_GetObjectItem(today, "textDay");
                char *textday = cJSON_GetStringValue(item);
                item = cJSON_GetObjectItem(today, "textNight");
                char *textnight = cJSON_GetStringValue(item);

                //将单词组成人能听的句子
                //今天气温16-20度，风力1-2级，白天多云，夜晚小雨
                snprintf(&str_weather[i][0], 100, "%s,气温%s至%s摄氏度,风力%s级,白天%s,夜晚%s.", time, tempmin, tempmax, windscale, textday, textnight);
                res = ESP_OK;
            }

            cJSON_Delete(root);
            
        }

        if (bit & UPDATE_TOKEN_BIT)
        {
            //更新token
            //ESP_LOGI(TAG, "UPDATE_TOKEN_BIT");
            memset(http_data, 0, MX_HTTP_BUFF);
            config.url = GET_TOKEN_URL;
            config.method = HTTP_METHOD_GET;

            ESP_LOGI(TAG,"start connect to url = %s\r\n", config.url);
            client = esp_http_client_init(&config);
            esp_http_client_perform(client);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);

            cJSON *root = cJSON_Parse(http_data);
            if (root != NULL)
            {
                cJSON *tok = cJSON_GetObjectItem(root, "access_token");
                char *newtoken = cJSON_GetStringValue(tok); //获取新token
                if (newtoken != NULL)
                {
                    memset(baidu_access_token, 0, 80);         //清零全局变量baidu_access_token
                    strncpy(baidu_access_token, newtoken, 80); //再更新
                    ESP_LOGI(TAG, "new token = %s", baidu_access_token);
                    res = ESP_OK;
                }
                else
                {
                    ESP_LOGI(TAG, "parse string error");
                    res = ESP_FAIL;
                }
            }
            else
            {
                ESP_LOGI(TAG, "root error");
                res = ESP_FAIL;
            }

            cJSON_Delete(root);
        }
        ESP_LOGI(TAG, "send to res_queue :%d", res);
        xQueueSend(res_queue, &res, portMAX_DELAY); //返回结果
    }

    vTaskDelete(NULL);
}

/*
 * http任务初始化
 * 创建http任务，获取网络时间 更新token
 */
void http_init()
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    res_queue = xQueueCreate(1, sizeof(int));
    http_api_evengroup = xEventGroupCreate();                             //创建api事件组，用于各种api请求的同步
    xTaskCreate(http_api_task, "http_api_task", 2048 * 2, NULL, 5, NULL); //创建http请求任务
    ESP_LOGI(TAG, "Http Init OK");
}