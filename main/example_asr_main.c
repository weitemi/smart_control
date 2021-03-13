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

#include "nvs_flash.h"
#include "nvs.h"

#include "buttonTask.h"

#include "mywifi.h"

#include "player.h"
#include "irTask.h"
#include "myhttp.h"
#include "myds18b20.h"
#include "clock.h"

#include "audio_pipeline.h"
#include "i2s_stream.h"
#include "raw_stream.h"
#include "esp_audio.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "mp3_decoder.h"
#include "filter_resample.h"
#include "rec_eng_helper.h"
#include "fatfs_stream.h"
#include "periph_button.h"
#include "board.h"
#include "esp_peripherals.h"
#include "periph_touch.h"
#include "periph_adc_button.h"

static const char *TAG = "main.c";

#define USE_HEAP_MANGER 0
/*----任务配置----*/
#define ASR_TASK_PRO 6
#define BUTTON_TASK_PRO 4
#define IR_TX_TASK_PRO 5
#define IR_RX_TASK_PRO 6
#define HTTP_GET_WEATHER_TASK_PRO 10
#define HEAP_MANAGER_TASK_PRO 20

#define ASR_TASK_SIZE 5120
#define BUTTON_TASK_SIZE 4096
#define IR_RX_TASK_SIZE 2048
#define IR_TX_TASK_SIZE 2048
#define HTTP_GET_WEATHER_TASK_SIZE 4096
#define HEAP_MANAGER_TASK_SIZE 2048

typedef enum
{
    WAKE_UP = 1,
} asr_wakenet_event_t;

typedef enum
{
    ID0_SHEZHIKONGTIAOERSHIDU = 0,
    ID1_SHEZHIKONGTIAOERSHIYIDU = 1,
    ID2_SHEZHIKONGTIAOERSHIERDU = 2,
    ID3_SHEZHIKONGTIAOERSHISANDU = 3,
    ID4_SHEZHIKONGTIAOERSHISIDU = 4,
    ID5_SHEZHIKONGTIAOERSHIWUDU = 5,
    ID6_SHEZHIKONGTIAOERSHLIUIDU = 6,
    ID7_SHEZHIKONGTIAOERSHIQIDU = 7,
    ID8_SHEZHIKONGTIAOERSHIBADU = 8,
    ID9_DAKAILANYA = 9,
    ID10_GUANBILANYA = 10,
    ID11_QIDONGKONGTIAOSAOFENG = 11,
    ID12_TINGZHIKONGTIAOSAOFENG = 12,
    ID13_SHEZHIKONGTIAOZIDONGFENGSU = 13,
    ID14_SHEZHIKONGTIAOYIJIFENGSU = 14,
    ID15_SHEZHIKONGTIAOERJIFENGSU = 15,
    ID16_SHEZHIKONGTIAOSANJIFENGSU = 16,
    ID17_MINGTIANTIANQIZENMEYANG = 17,
    ID18_JINTIANTIANQIZENMEYANG = 18,
    ID19_YIXIAOSHIHOUGUANBIKONGTIAO = 19,
    ID20_LIANGXIAOSHIHOUGUANBIKONGTIAO = 20,
    ID21_SANXIAOSHIHOUGUANBIKONGTIAO = 21,
    ID22_SIXIAOSHIHOUGUANBIKONGTIAO = 22,
    ID23_WUXIAOSHIHOUGUANBIKONGTIAO = 23,
    ID24_LIUXIAOSHIHOUGUANBIKONGTIAO = 24,
    ID25_DAKAIKONGTIAO = 25,
    ID26_GUANBIKONGTIAO = 26,
    ID27_QIXIAOSHIHOUGUANBIKONGTIAO = 27,
    ID28_BAXIAOSHIHOUGUANBIKONGTIAO = 28,
    ID29_JIUXIAOSHIHOUGUANBIKONGTIAO = 29,
    ID30_SHIXIAOSHIHOUGUANBIKONGTIAO = 30,
    ID31_SHINEIWENDU = 31,
    ID32_HOUTIANTIANQIZENMEYANG = 32,
    ID33_SHIMIAOHOUGUANBIKONGTIAO = 33,
    ID34_XIANZAIJIDIAN = 34,
    ID35_WUMIAOHOUGUANBIKONGTIAO = 35,
    ID36_LIUMIAOHOUGUANBIKONGTIAO = 36,
    ID37_KAIQIHONGWAIXUEXI = 37,
    ID38_HONGWAIXUEXI = 38,
    ID_MAX,
} asr_multinet_event_t;

static esp_err_t asr_multinet_control(int commit_id);
void heap_manager_task(void *agr);

/*
 * 板子所有io的初始化
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
void app_main()
{

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    led_init();
    storage_init();
    player_init();
    xTaskCreate(button_task, "btn", 2048, NULL, 5, NULL);
    IR_init();
    xTaskCreate(rmt_ir_txTask, "ir_tx", IR_TX_TASK_SIZE, NULL, IR_TX_TASK_PRO, &ir_tx_handle);

    xTaskCreate(rmt_ir_rxTask, "ir_rx", IR_RX_TASK_SIZE, NULL, IR_RX_TASK_PRO, &ir_rx_handle);

    
    //wifi_init_sta();
    //httptask_init();
    //xTaskCreate(clock_task, "clock_Task", IR_TX_TASK_SIZE, NULL, IR_TX_TASK_PRO, NULL); //不完善

#if USE_HEAP_MANGER
    xTaskCreate(heap_manager_task, "heap_manager_task", HEAP_MANAGER_TASK_SIZE, NULL, HEAP_MANAGER_TASK_PRO, NULL);
#endif


    ESP_LOGI(TAG, "Initialize SR wn handle");
    esp_wn_iface_t *wakenet;
    model_coeff_getter_t *model_coeff_getter;
    model_iface_data_t *model_wn_data;
    const esp_mn_iface_t *multinet = &MULTINET_MODEL;

    // Initialize wakeNet model data
    get_wakenet_iface(&wakenet);
    get_wakenet_coeff(&model_coeff_getter);
    model_wn_data = wakenet->create(model_coeff_getter, DET_MODE_90);

    int wn_num = wakenet->get_word_num(model_wn_data);
    for (int i = 1; i <= wn_num; i++)
    {
        char *name = wakenet->get_word_name(model_wn_data, i);
        ESP_LOGI(TAG, "keywords: %s (index = %d)", name, i);
    }
    float wn_threshold = wakenet->get_det_threshold(model_wn_data, 1);
    int wn_sample_rate = wakenet->get_samp_rate(model_wn_data);
    int audio_wn_chunksize = wakenet->get_samp_chunksize(model_wn_data);
    ESP_LOGI(TAG, "keywords_num = %d, threshold = %f, sample_rate = %d, chunksize = %d, sizeof_uint16 = %d", wn_num, wn_threshold, wn_sample_rate, audio_wn_chunksize, sizeof(int16_t));

    model_iface_data_t *model_mn_data = multinet->create(&MULTINET_COEFF, 4000); //语音识别时间，single模式下最大5s
    int audio_mn_chunksize = multinet->get_samp_chunksize(model_mn_data);
    int mn_num = multinet->get_samp_chunknum(model_mn_data);
    int mn_sample_rate = multinet->get_samp_rate(model_mn_data);
    ESP_LOGI(TAG, "keywords_num = %d , sample_rate = %d, chunksize = %d, sizeof_uint16 = %d", mn_num, mn_sample_rate, audio_mn_chunksize, sizeof(int16_t));

    int size = audio_wn_chunksize;
    if (audio_mn_chunksize > audio_wn_chunksize)
    {
        size = audio_mn_chunksize;
    }
    int16_t *buffer = (int16_t *)malloc(size * sizeof(short));

    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_reader, filter, raw_read;

    bool enable_wn = true;
    uint32_t mn_count = 0;

    ESP_LOGI(TAG, "[ 1 ] Start codec chip");


    ESP_LOGI(TAG, "[ 2.0 ] Create audio pipeline for recording");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[ 2.1 ] Create i2s stream to read audio data from codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.i2s_config.sample_rate = 48000;
    i2s_cfg.type = AUDIO_STREAM_READER;

    i2s_stream_reader = i2s_stream_init(&i2s_cfg);
    ESP_LOGI(TAG, "[ 2.2 ] Create filter to resample audio data");
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = 48000;
    rsp_cfg.src_ch = 2;
    rsp_cfg.dest_rate = 16000;
    rsp_cfg.dest_ch = 1;
    filter = rsp_filter_init(&rsp_cfg);

    ESP_LOGI(TAG, "[ 2.3 ] Create raw to receive data");
    raw_stream_cfg_t raw_cfg = {
        .out_rb_size = 8 * 1024,
        .type = AUDIO_STREAM_READER,
    };
    raw_read = raw_stream_init(&raw_cfg);

    ESP_LOGI(TAG, "[ 3 ] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s");
    audio_pipeline_register(pipeline, raw_read, "raw");

    audio_pipeline_register(pipeline, filter, "filter");
    ESP_LOGI(TAG, "[ 4 ] Link elements together [codec_chip]-->i2s_stream-->filter-->raw-->[SR]");
    const char *link_tag[3] = {"i2s", "filter", "raw"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);

    ESP_LOGI(TAG, "[ 5 ] waiting to be awake");
    audio_pipeline_run(pipeline);

    while (1)
    {
        raw_stream_read(raw_read, (char *)buffer, size * sizeof(short));
        if (enable_wn)
        {
            if (wakenet->detect(model_wn_data, (int16_t *)buffer) == WAKE_UP)
            {

                gpio_set_level(GPIO_NUM_18, 0);
                ESP_LOGI(TAG, "wake up");
                enable_wn = false;
            }
        }
        else
        {
            mn_count++;
            int commit_id = multinet->detect(model_mn_data, buffer);
            if (asr_multinet_control(commit_id) == ESP_OK)
            {
                gpio_set_level(GPIO_NUM_18, 1);
                enable_wn = true;
                mn_count = 0;
            }
            if (mn_count == mn_num)
            {
                ESP_LOGI(TAG, "stop multinet");
                gpio_set_level(GPIO_NUM_18, 1);
                enable_wn = true;
                mn_count = 0;
            }
        }
    }

    ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");

    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    audio_pipeline_terminate(pipeline);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    audio_pipeline_unregister(pipeline, raw_read);
    audio_pipeline_unregister(pipeline, i2s_stream_reader);
    audio_pipeline_unregister(pipeline, filter);

    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(raw_read);
    audio_element_deinit(i2s_stream_reader);
    audio_element_deinit(filter);

    ESP_LOGI(TAG, "[ 7 ] Destroy model");
    wakenet->destroy(model_wn_data);
    model_wn_data = NULL;
    free(buffer);
    buffer = NULL;
}
static char *weather, s[25] = {0};
static float temp;
static clk_t clk;
/*
 * 空调定时关闭回调函数
 */
timer_cb ir_close_cb(struct timer *tmr, void *arg)
{
    ESP_LOGI(TAG, "timer to close the aircon");
    ac_open(false);

    return NULL;
}
/*
 * 空调定时关闭回调函数 5s测试专用
 */
timer_cb cb_5s(struct timer *tmr, void *arg)
{
    ESP_LOGI(TAG, "timer to close the aircon");
ac_open(true);
    AC_SUCCESS_MP3;
    return NULL;
}
/*
 * 空调定时开启回调函数 6s测试专用
 */
timer_cb cb_6s(struct timer *tmr, void *arg)
{
    ESP_LOGI(TAG, "timer to close the aircon");
    ac_set_temp(26);
    AC_SUCCESS_MP3;
    return NULL;
}

static esp_err_t asr_multinet_control(int commit_id)
{
    if (commit_id >= 0 && commit_id < ID_MAX)
    {
        switch (commit_id)
        {
        case ID0_SHEZHIKONGTIAOERSHIDU:
            ac_set_temp(20);

            ESP_LOGI(TAG, "ID0_SHEZHIKONGTIAOERSHIDU");
            break;

        case ID1_SHEZHIKONGTIAOERSHIYIDU:
            ac_set_temp(21);

            ESP_LOGI(TAG, "ID1_SHEZHIKONGTIAOERSHIYIDU");
            break;

        case ID2_SHEZHIKONGTIAOERSHIERDU:
            ac_set_temp(22);

            ESP_LOGI(TAG, "ID2_SHEZHIKONGTIAOERSHIERDU");
            break;

        case ID3_SHEZHIKONGTIAOERSHISANDU:
            ac_set_temp(23);

            ESP_LOGI(TAG, "ID3_SHEZHIKONGTIAOERSHISANDU");
            break;

        case ID4_SHEZHIKONGTIAOERSHISIDU:
            ac_set_temp(24);

            ESP_LOGI(TAG, "ID4_SHEZHIKONGTIAOERSHISIDU");
            break;

        case ID5_SHEZHIKONGTIAOERSHIWUDU:
            ac_set_temp(25);

            ESP_LOGI(TAG, "ID5_SHEZHIKONGTIAOERSHIWUDU");
            break;

        case ID6_SHEZHIKONGTIAOERSHLIUIDU:
            ac_set_temp(26);
            ESP_LOGI(TAG, "ID6_SHEZHIKONGTIAOERSHLIUIDU");
            break;

        case ID7_SHEZHIKONGTIAOERSHIQIDU:
            ac_set_temp(27);

            ESP_LOGI(TAG, "ID7_SHEZHIKONGTIAOERSHIQIDU");
            break;

        case ID8_SHEZHIKONGTIAOERSHIBADU:
            ac_set_temp(28);

            ESP_LOGI(TAG, "ID8_SHEZHIKONGTIAOERSHIBADU");
            break;

        case ID9_DAKAILANYA:
            //todo lanya
            ESP_LOGI(TAG, "ID9_DAKAILANYA");
            break;
        case ID10_GUANBILANYA:
            ESP_LOGI(TAG, "ID10_GUANBILANYA");
            break;
        case ID11_QIDONGKONGTIAOSAOFENG:
            //todo :saofeng?
            ac_set_swing(true);
            ESP_LOGI(TAG, "ID11_QIDONGKONGTIAOSAOFENG");
            break;

        case ID12_TINGZHIKONGTIAOSAOFENG:
            ac_set_swing(false);
            ESP_LOGI(TAG, "ID12_TINGZHIKONGTIAOSAOFENG");
            break;

        case ID13_SHEZHIKONGTIAOZIDONGFENGSU:
            ac_set_wind_speed(0);
            ESP_LOGI(TAG, "ID13_SHEZHIKONGTIAOZIDONGFENGSU");
            break;

        case ID14_SHEZHIKONGTIAOYIJIFENGSU:
            ac_set_wind_speed(1);

            ESP_LOGI(TAG, "ID14_SHEZHIKONGTIAOYIJIFENGSU");
            break;

        case ID15_SHEZHIKONGTIAOERJIFENGSU:
            ac_set_wind_speed(2);

            ESP_LOGI(TAG, "ID15_SHEZHIKONGTIAOERJIFENGSU");
            break;

        case ID16_SHEZHIKONGTIAOSANJIFENGSU:
            ac_set_wind_speed(3);

            ESP_LOGI(TAG, "ID16_SHEZHIKONGTIAOSANJIFENGSU");
            break;

        case ID17_MINGTIANTIANQIZENMEYANG:
            weather = get_Weather_String(1);
            speech_sync(weather);
            ESP_LOGI(TAG, "ID17_MINGTIANTIANQIZENMEYANG");
            break;
        case ID18_JINTIANTIANQIZENMEYANG:
            weather = get_Weather_String(0);
            speech_sync(weather);
            ESP_LOGI(TAG, "ID18_JINTIANTIANQIZENMEYANG");
            break;
        case ID19_YIXIAOSHIHOUGUANBIKONGTIAO:
            ESP_LOGI(TAG, "ID19_YIXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 1;
            goto timer;

        case ID20_LIANGXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID20_LIANGXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 2;
            goto timer;

        case ID21_SANXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID21_SANXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 3;
            goto timer;

        case ID22_SIXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID22_SIXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 4;
            goto timer;

        case ID23_WUXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID23_WUXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 5;
            goto timer;

        case ID24_LIUXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID24_LIUXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 6;
            goto timer;

        case ID25_DAKAIKONGTIAO:
            ESP_LOGI(TAG, "ID25_DAKAIKONGTIAO");
            ac_open(true);
            break;

        case ID26_GUANBIKONGTIAO:
            ac_open(false);
            AC_CLOSE_MP3;
            ESP_LOGI(TAG, "ID26_GUANBIKONGTIAO");
            break;
        case ID27_QIXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID27_QIXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 7;
            goto timer;

        case ID28_BAXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID28_BAXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 8;
            goto timer;

        case ID29_JIUXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID29_JIUXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 9;
            goto timer;

        case ID30_SHIXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID30_SHIXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 10;
            goto timer;

        case ID31_SHINEIWENDU:
            ESP_LOGI(TAG, "ID31_SHINEIWENDU ");
            temp = ds18b20_get_data();

            sprintf(s, "当前室温 %.1f 度", temp);
            speech_sync(s);

            break;
        case ID32_HOUTIANTIANQIZENMEYANG:
            ESP_LOGI(TAG, "ID32_HOUTIANTIANQIZENMEYANG");
            weather = get_Weather_String(2);
            speech_sync(weather);

            break;
        case ID33_SHIMIAOHOUGUANBIKONGTIAO:
            ESP_LOGI(TAG, "ID33_SHIMIAOHOUGUANBIKONGTIAO");
            SETTMR_MP3;

            break;
        case ID34_XIANZAIJIDIAN:
            ESP_LOGI(TAG, "ID34_XIANZAIJIDIAN:%s", s);
            sprintf(s, "20%d-%d-%d %d:%d:%d\r\n", global_clk.cal.year, global_clk.cal.month, global_clk.cal.date, global_clk.cal.hour, global_clk.cal.minute, global_clk.cal.second);
            speech_sync(s);

            break;
        case ID35_WUMIAOHOUGUANBIKONGTIAO:
            ESP_LOGI(TAG, "ID35_WUMIAOHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.second += 5;
            tmr_new(&clk, cb_5s, NULL, "5s");
            SETTMR_MP3;
            
            break;
        case ID36_LIUMIAOHOUGUANBIKONGTIAO:
            ESP_LOGI(TAG, "ID36_LIUMIAOHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.second += 6;
            tmr_new(&clk, cb_6s, NULL, "6s");
            SETTMR_MP3;
            
            break;
        case ID37_KAIQIHONGWAIXUEXI:

            ESP_LOGI(TAG, "ID37_KAIQIHONGWAIXUEXI");
            break;
        case ID38_HONGWAIXUEXI:
            ESP_LOGI(TAG, "please send message");
            ir_study();
            break;
        default:
            ESP_LOGI(TAG, "not supportint mode");
            break;
        }

        return ESP_OK;
    }
    return ESP_FAIL;

timer:
    tmr_new(&clk, ir_close_cb, NULL, "ir_close");
    SETTMR_MP3;
    return ESP_OK;
}



////////////////////////////////////////////////////////////////////

#if USE_HEAP_MANGER
/* 
 * 堆栈管理任务
 * 打印各个任务的堆栈使用情况
 */
void heap_manager_task(void *agr)
{
    const char *TAG = "heap_manger";

    esp_log_level_set(TAG, ESP_LOG_INFO);

    while (1)
    {
        vTaskDelay(10000 / portTICK_RATE_MS); //10s

        //printf("---heap manger---\r\n");
        //打印任务的堆栈情况
        //printf("%-10.10s,free heap:%d\r\n",pcTaskGetTaskName(buttonTask_handle),uxTaskGetStackHighWaterMark(buttonTask_handle));
        //printf("%-10.10s,free heap:%d\r\n",pcTaskGetTaskName(ir_tx_handle),uxTaskGetStackHighWaterMark(ir_tx_handle));
        //printf("%-10.10s,free heap:%d\r\n",pcTaskGetTaskName(ir_rx_handle),uxTaskGetStackHighWaterMark(ir_rx_handle));
        //printf("%-10.10s,free heap:%d\r\n",pcTaskGetTaskName(ASR_task_handle),uxTaskGetStackHighWaterMark(ASR_task_handle));

        //打印全局时间
        printf("20%d-%d-%d %d:%d:%d\r\n", global_clk.cal.year, global_clk.cal.month, global_clk.cal.date, global_clk.cal.hour, global_clk.cal.minute, global_clk.cal.second);
        printf_tmrlist();
        //printf("---heap manger---\r\n");
    }
}

#endif


#if 0
static void all_init()
{
    #if 1
    gpio_config_t led;
    led.mode = GPIO_MODE_OUTPUT;
    led.pin_bit_mask = (1ULL << LED);
    led.intr_type = GPIO_INTR_DISABLE;
    led.pull_down_en = 0;
    led.pull_up_en = 0;
    gpio_config(&led);
    gpio_set_level(GPIO_NUM_18, 1);




    storage_init();
    //wifi_init_sta();
    //player_init();
    IR_init();
    xTaskCreate(rmt_ir_txTask, "ir_tx", IR_TX_TASK_SIZE, NULL, IR_TX_TASK_PRO, &ir_tx_handle);

    xTaskCreate(rmt_ir_rxTask, "ir_rx", IR_RX_TASK_SIZE, NULL, IR_RX_TASK_PRO, &ir_rx_handle);

    //xTaskCreate(clock_task, "clock_Task", IR_TX_TASK_SIZE, NULL, IR_TX_TASK_PRO, NULL); //不完善

    
    //httptask_init();
    #endif
    
}


void app_main()
{


#if USE_HEAP_MANGER
    xTaskCreate(heap_manager_task, "heap_manager_task", HEAP_MANAGER_TASK_SIZE, NULL, HEAP_MANAGER_TASK_PRO, NULL);
#endif
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "[ 1 ] Start audio codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    int player_volume;
    audio_hal_get_volume(board_handle->audio_hal, &player_volume);


    ESP_LOGI(TAG, "[ 3 ] Initialize peripherals");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    
    ESP_LOGI(TAG, "[3.1] Initialize keys on board");
    audio_board_key_init(set);

    ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);


    ESP_LOGI(TAG, "[4.2] Listening event from peripherals");
    audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

    all_init();
    ir_study();
    xTaskCreate(ASR_Task, "ASR_Task", ASR_TASK_SIZE, NULL, ASR_TASK_PRO, NULL);
    uint32_t lib_num = 0;
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
                //ac_set_code_lib(band_haier, lib_num);
                //ac_open(true);
                //ESP_LOGI(TAG, "ir tx:%s", ir_code_lib[band_haier*5+lib_num]);
                ESP_LOGI(TAG, "please send message");
                ir_study();
            }
            else if ((int)msg.data == KEY1)
            {
                
                ac_set_temp(25);
#if 0
                lib_num++;
                if(lib_num==5)
                {
                    lib_num = 0;
                }
                ESP_LOGI(TAG, "lib num = %u",lib_num);
#endif
            }
        }
    }
}
#endif