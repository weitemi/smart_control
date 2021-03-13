/*
 * @Author: your name
 * @Date: 2020-10-25 15:01:11
 * @LastEditTime: 2021-03-14 00:21:57
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\ASR\main\periph\buttonTask.c
 */
#include "buttonTask.h"
#include "irTask.h"
#include "dht11.h"
#include "myhttp.h"
#include "player.h"
#include "myds18b20.h"
#include "clock.h"
#include "driver/gpio.h"

static const char *TAG = "buttonTask";


TaskHandle_t buttonTask_handle;




#if 1
/*
* 按键任务
*/
void button_task(void *arg)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, " Initialize peripherals");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

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

                ESP_LOGI(TAG, "k2 :send message:");
                ir_study();
            }
            else if ((int)msg.data == KEY1)
            {

                 ESP_LOGI(TAG, "k1");
            }
        }
    }
}
#endif
