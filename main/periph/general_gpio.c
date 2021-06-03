/*
 * @Author: your name
 * @Date: 2020-10-25 15:01:11
 * @LastEditTime: 2021-06-03 23:49:53
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\ASR\main\periph\buttonTask.c
 */
#include "general_gpio.h"
#include "irTask.h"

#include "myhttp.h"
#include "myble.h"
#include "clock.h"

static const char *TAG = "General_Gpio";
audio_event_iface_handle_t evt;

/*
* 按键任务
*/
void button_task(void *arg)
{
    while (1)
    {
        audio_event_iface_msg_t msg;
        clk_t t;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if ((msg.source_type == PERIPH_ID_BUTTON) && ( msg.cmd == PERIPH_BUTTON_PRESSED ))
        {

            if ((int)msg.data == KEY2)
            {
                
                //ESP_LOGI(TAG, "k2 :send message:");
                //ir_study();     //红外学习
                //ble_close();
                t.value = get_clk();
                ESP_LOGI(TAG, "now time %d",t.cal.second);
            }
            else if ((int)msg.data == KEY1)
            {
                ble_open();
            }
        }
    }
}

/*
 * 通用gpio初始化
 * 初始化led引脚配置，按键配置，创建按键检测任务
 */
int General_Gpio_init(esp_periph_set_handle_t set)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "init gpio");
    esp_err_t err = 0;
    gpio_config_t led;

    led.mode = GPIO_MODE_OUTPUT;
    led.pin_bit_mask = (1ULL << LED);
    led.intr_type = GPIO_INTR_DISABLE;
    led.pull_down_en = 0;
    led.pull_up_en = 0;

    err = gpio_config(&led);
    if (err != ESP_OK)
    {
        ESP_LOGI(TAG, "led fail");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, " Initialize keys on board");
    audio_board_key_init(set);

    ESP_LOGI(TAG, " Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    evt = audio_event_iface_init(&evt_cfg);
    
    ESP_LOGI(TAG, " Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);
    
    xTaskCreate(button_task, "btn", BUTTON_TASK_SIZE, NULL, BUTTON_TASK_PRO, NULL);
    
    LED_OFF; 
    
    return ESP_OK;
}