#include "mywifi.h"
#include "myhttp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "mymqtt.h"

#define EXAMPLE_ESP_WIFI_SSID "esp"
#define EXAMPLE_ESP_WIFI_PASS "07685812870"
#define MAXIMUM_RETRY 20

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect(); //开始wifi连接
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
   
        //wifi断开，清除连接标志
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        //todo 此处应有led表示wifi状态

        //尝试重连
        if (s_retry_num < MAXIMUM_RETRY)
        {
            //重新连接
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP...");
            
        }
        else
        {
            //重连次数超过
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        //todo 获取到ip地址，此处应有led指示 

        //获取ip事件数据 并打印ip信息
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        
        //连接到wifi后 获取全局时钟，更新百度token
        update_access_token();

        mqtt_init();
    }
}

/*
 * wifi初始化为station模式
 * 注册事件组，可断线重连
 */
void wifi_init_sta()
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();   //安装tcpip适配器

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //注册wifi/ip事件回调函数
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    //设置wifi为station模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    //!以下语句注释掉，防止每次上电都要连接代码中的wifi配置
    //ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

}

/*
 * 更新wifi配置 用于更换路由器
 * ssid:ap名称
 * password:密码
 * 返回：ESP_FAIL OR ESP_OK
*/
int wifi_update(const char *ssid,const char *password)
{
    wifi_config_t wifi_config = {
        .sta = {
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };
    if(ssid==NULL||password==NULL)
    {
        return ESP_FAIL;
    }
    //使用内存复制
    strncpy((char *)wifi_config.sta.ssid, ssid, strlen(ssid));
    strncpy((char *)wifi_config.sta.password, password, strlen(password));

    ESP_LOGI(TAG, "update wifi ssid:%s  password:%s", wifi_config.sta.ssid, wifi_config.sta.password);
    //若wifi已连接则先断开
    if (get_wifi_status())
    {
        ESP_ERROR_CHECK(esp_wifi_disconnect());
    }
    s_retry_num = 0;
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config)); //根据wifi_config重新设置wifi信息

    return esp_wifi_connect(); //重新连接


}
/*
 * wifi连接状态查询
 * 返回：1：连接 0：未连接
 */
int get_wifi_status()
{
    //根据wifi事件组的标志位判断连接状态
    EventBits_t wifi_event_bit = xEventGroupGetBits(s_wifi_event_group);
    if (wifi_event_bit & WIFI_CONNECTED_BIT)
    {
        return 1;
    }
    else
        return 0;
}

//todo ：测试
wifi_csi_cb_t csi_cb(void *ctx, wifi_csi_info_t *data)
{
    return NULL;
}
void wifi_csi_init()
{
    wifi_csi_config_t conf;
    conf.channel_filter_en = true;
    conf.htltf_en = true;
    conf.lltf_en = true;
    conf.ltf_merge_en = true;
    conf.manu_scale = false;
    conf.stbc_htltf2_en = true;
    conf.shift = 0;
    if (esp_wifi_set_csi_config(&conf) != ESP_OK)
    {
        ESP_LOGI(TAG, "wifi csi err");
    }
    esp_wifi_set_csi_rx_cb(csi_cb, NULL);
}
