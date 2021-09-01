/*
 * @Author: your name
 * @Date: 2021-09-01 20:52:35
 * @LastEditTime: 2021-09-01 21:45:44
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \smart_control\main\main.c
 */
/* Examples of speech recognition with multiple keywords.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "board.h"
#include "esp_system.h"

//periph includes
#include "general_gpio.h"
#include "storage.h"
#include "mywifi.h"
#include "ir.h"
#include "myds18b20.h"
#include "myuart.h"

//systerm includes
#include "clock.h"

//audio includes
#include "player.h"

//network includes
#include "myhttp.h"
#include "myble.h"
#include "mymqtt.h"
#include "asrtask.h"
static const char *TAG = "main";


void app_main()
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    General_Gpio_init(set); 
    uart_init();
    IR_init();
    ds18b20_get_data(); //首次上电需要读取一次,后续测量才会准确
    storage_init();
    player_init();
    wifi_init_sta();
    ble_init();
    http_init();
    clk_init();
    ASR_Init();
    while (1)
    {
        vTaskDelay(1000);
    }
}
