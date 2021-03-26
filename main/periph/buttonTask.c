/*
 * @Author: your name
 * @Date: 2020-10-25 15:01:11
 * @LastEditTime: 2021-03-26 09:17:40
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\ASR\main\periph\buttonTask.c
 */
#include "buttonTask.h"
#include "irTask.h"
#include "esp_peripherals.h"
#include "periph_touch.h"
#include "periph_adc_button.h"
#include "periph_button.h"
#include "esp_log.h"
#include "board.h"
#include "mynvs.h"
#include "driver/gpio.h"
#include "myhttp.h"
#include "myble.h"


static const char *TAG = "buttonTask";




/*
* 按键任务
*/
void button_task(void *arg)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    esp_periph_set_handle_t set = (esp_periph_set_handle_t)(arg);

    ESP_LOGI(TAG, " Initialize keys on board");
    audio_board_key_init(set);

    ESP_LOGI(TAG, " Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, " Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    while (1)
    {

        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if ((msg.source_type == PERIPH_ID_TOUCH || msg.source_type == PERIPH_ID_BUTTON || msg.source_type == PERIPH_ID_ADC_BTN) && (msg.cmd == PERIPH_TOUCH_TAP || msg.cmd == PERIPH_BUTTON_PRESSED || msg.cmd == PERIPH_ADC_BUTTON_PRESSED))
        {

            if ((int)msg.data == KEY2)
            {
                
                //ESP_LOGI(TAG, "k2 :send message:");
                //ir_study();     //红外学习
                ble_close();
            }
            else if ((int)msg.data == KEY1)
            {
                ble_open();
            }
        }
    }
}

/*
 * led的初始化
 */
int led_init()
{
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
    gpio_set_level(GPIO_NUM_18, 1);
    return ESP_OK;
}