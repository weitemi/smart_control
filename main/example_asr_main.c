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
#include "esp_wifi.h"
#include "periph_wifi.h"
#include "esp_event.h"
#include "myuart.h"

#include "lwip/sockets.h"
#include "myble.h"

static const char *TAG = "main.c";

#define USE_HEAP_MANGER 1
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

#define HOST_IP_ADDR "192.168.3.1"
#define PORT "82"

typedef enum
{
    WAKE_UP = 1,
} asr_wakenet_event_t;

typedef enum
{
    //空调
    ID0_SHEZHIKONGTIAOERSHIDU = 0,
    ID1_SHEZHIKONGTIAOERSHIYIDU = 1,
    ID2_SHEZHIKONGTIAOERSHIERDU = 2,
    ID3_SHEZHIKONGTIAOERSHISANDU = 3,
    ID4_SHEZHIKONGTIAOERSHISIDU = 4,
    ID5_SHEZHIKONGTIAOERSHIWUDU = 5,
    ID6_SHEZHIKONGTIAOERSHLIUIDU = 6,
    ID7_SHEZHIKONGTIAOERSHIQIDU = 7,
    ID8_SHEZHIKONGTIAOERSHIBADU = 8,
    ID9_QIDONGKONGTIAOSAOFENG = 9,
    ID10_TINGZHIKONGTIAOSAOFENG = 10,
    ID11_SHEZHIKONGTIAOZIDONGFENGSU = 11,
    ID12_SHEZHIKONGTIAOYIJIFENGSU = 12,
    ID13_SHEZHIKONGTIAOERJIFENGSU = 13,
    ID14_SHEZHIKONGTIAOSANJIFENGSU = 14,
    ID15_YIXIAOSHIHOUGUANBIKONGTIAO = 15,
    ID16_LIANGXIAOSHIHOUGUANBIKONGTIAO = 16,
    ID17_SANXIAOSHIHOUGUANBIKONGTIAO = 17,
    ID18_SIXIAOSHIHOUGUANBIKONGTIAO = 18,
    ID19_WUXIAOSHIHOUGUANBIKONGTIAO = 19,
    ID20_LIUXIAOSHIHOUGUANBIKONGTIAO = 20,
    ID21_QIXIAOSHIHOUGUANBIKONGTIAO = 21,
    ID22_BAXIAOSHIHOUGUANBIKONGTIAO = 22,
    ID23_JIUXIAOSHIHOUGUANBIKONGTIAO = 23,
    ID24_SHIXIAOSHIHOUGUANBIKONGTIAO = 24,
    ID25_DAKAIKONGTIAO = 25,
    ID26_GUANBIKONGTIAO = 26,

    //蓝牙
    ID30_DAKAILANYA = 30,
    ID31_GUANBILANYA = 31,

    //天气
    ID32_MINGTIANTIANQIZENMEYANG = 32,
    ID33_JINTIANTIANQIZENMEYANG = 33,
    ID34_HOUTIANTIANQIZENMEYANG = 34,

    //其他
    ID35_SHINEIWENDU = 35,
    ID36_XIANZAIJIDIAN = 36,
    ID37_HONGWAIXUEXI = 37,

    //测试
    ID40_SHIMIAOHOUGUANBIKONGTIAO = 40,
    ID41_JIUMIAOHOUDAKAIKONGTIAO = 41,

    ID_MAX,
} asr_multinet_event_t;

static esp_err_t asr_multinet_control(int commit_id);
void heap_manager_task(void *agr);

void app_main()
{

    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_INFO);
    esp_err_t err;

    uart_init();
    storage_init();
    led_init();

    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    player_init();

    xTaskCreate(button_task, "btn", BUTTON_TASK_SIZE, set, BUTTON_TASK_PRO, NULL);

    IR_init();
    xTaskCreate(rmt_ir_txTask, "ir_tx", IR_TX_TASK_SIZE, NULL, IR_TX_TASK_PRO, &ir_tx_handle);

    xTaskCreate(rmt_ir_rxTask, "ir_rx", IR_RX_TASK_SIZE, NULL, IR_RX_TASK_PRO, NULL);


    ds18b20_get_data(); //首次上电需要读取一次

    httptask_init();

    ble_start();
    wifi_init_sta();

    xTaskCreate(clock_task, "clock_Task", IR_TX_TASK_SIZE, NULL, IR_TX_TASK_PRO, NULL);

#if USE_HEAP_MANGER
    xTaskCreate(heap_manager_task, "heap_manager_task", HEAP_MANAGER_TASK_SIZE, NULL, HEAP_MANAGER_TASK_PRO, NULL);
#endif

    ESP_LOGI(TAG, "Initialize SR wn handle");
    esp_wn_iface_t *wakenet;    //唤醒模型
    model_coeff_getter_t *model_coeff_getter;   //神经网络系数获取
    model_iface_data_t *model_wn_data;  //识别模型的数据
    const esp_mn_iface_t *multinet = &MULTINET_MODEL;   //识别模型

    // Initialize wakeNet model data
    get_wakenet_iface(&wakenet);    //初始化唤醒模型
    get_wakenet_coeff(&model_coeff_getter); //获取系数
    model_wn_data = wakenet->create(model_coeff_getter, DET_MODE_90);   //创建唤醒模型，设置灵敏度90%

    int wn_num = wakenet->get_word_num(model_wn_data);  //唤醒词数量
    for (int i = 1; i <= wn_num; i++)
    {
        char *name = wakenet->get_word_name(model_wn_data, i);  //唤醒词文本
        ESP_LOGI(TAG, "keywords: %s (index = %d)", name, i);
    }
    float wn_threshold = wakenet->get_det_threshold(model_wn_data, 1);  //获取唤醒阈值
    int wn_sample_rate = wakenet->get_samp_rate(model_wn_data); //唤醒词采样率16k
    int audio_wn_chunksize = wakenet->get_samp_chunksize(model_wn_data);    //内存块大小
    ESP_LOGI(TAG, "keywords_num = %d, threshold = %f, sample_rate = %d, chunksize = %d, sizeof_uint16 = %d", wn_num, wn_threshold, wn_sample_rate, audio_wn_chunksize, sizeof(int16_t));

    model_iface_data_t *model_mn_data = multinet->create(&MULTINET_COEFF, 4000); //语音识别时间，single模式下最大5s
    int audio_mn_chunksize = multinet->get_samp_chunksize(model_mn_data);   //识别内存块
    int mn_num = multinet->get_samp_chunknum(model_mn_data);    //唤醒词数量
    int mn_sample_rate = multinet->get_samp_rate(model_mn_data);    //采样率16k
    ESP_LOGI(TAG, "keywords_num = %d , sample_rate = %d, chunksize = %d, sizeof_uint16 = %d", mn_num, mn_sample_rate, audio_mn_chunksize, sizeof(int16_t));

    //选择所需的较大的内存块
    int size = audio_wn_chunksize;
    if (audio_mn_chunksize > audio_wn_chunksize)
    {
        size = audio_mn_chunksize;
    }
    int16_t *buffer = (int16_t *)malloc(size * sizeof(short));  //buffer用于缓存经过流水线处理的音频

    /*[ac101]-->i2s_stream-->filter-->raw-->[SR]*/
    audio_pipeline_handle_t pipeline;   //音频输入流水线
    audio_element_handle_t i2s_stream_reader, filter, raw_read; //流水线车间

    bool enable_wn = true;  //唤醒使能
    uint32_t mn_count = 0;

    ESP_LOGI(TAG, "[ 1 ] Start codec chip");

    ESP_LOGI(TAG, "[ 2.0 ] Create audio pipeline for recording");
    //流水线初始化
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    //i2s初始化，用于与ac101通信
    ESP_LOGI(TAG, "[ 2.1 ] Create i2s stream to read audio data from codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.i2s_config.sample_rate = 48000;
    i2s_cfg.type = AUDIO_STREAM_READER; //输入流
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[ 2.2 ] Create filter to resample audio data");
    //滤波器初始化，将源采样率变为16k
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = 48000;
    rsp_cfg.src_ch = 2;
    rsp_cfg.dest_rate = 16000;
    rsp_cfg.dest_ch = 1;
    filter = rsp_filter_init(&rsp_cfg);

    //raw初始化，缓存经过处理的音频数据
    ESP_LOGI(TAG, "[ 2.3 ] Create raw to receive data");
    raw_stream_cfg_t raw_cfg = {
        .out_rb_size = 8 * 1024,
        .type = AUDIO_STREAM_READER,    //输入流
    };
    raw_read = raw_stream_init(&raw_cfg);

    //将各个车间流连接到流水线
    ESP_LOGI(TAG, "[ 3 ] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s");
    audio_pipeline_register(pipeline, raw_read, "raw");
    audio_pipeline_register(pipeline, filter, "filter");
    ESP_LOGI(TAG, "[ 4 ] Link elements together [codec_chip]-->i2s_stream-->filter-->raw-->[SR]");
    const char *link_tag[3] = {"i2s", "filter", "raw"};
    audio_pipeline_link(pipeline, &link_tag[0], 3);

    //运行流水线
    ESP_LOGI(TAG, "[ 5 ] waiting to be awake");
    audio_pipeline_run(pipeline);

    while (1)
    {
        //读取raw的音频到buffer
        raw_stream_read(raw_read, (char *)buffer, size * sizeof(short));
        if (enable_wn)
        {
            //检测buffer是否有唤醒词
            if (wakenet->detect(model_wn_data, (int16_t *)buffer) == WAKE_UP)
            {

                LED_ON;
                ESP_LOGI(TAG, "wake up");
                enable_wn = false;
            }
        }
        else
        {
            mn_count++;
            //检测buffer中是否有命令词
            int commit_id = multinet->detect(model_mn_data, buffer);
            //进入命令词控制函数
            if (asr_multinet_control(commit_id) == ESP_OK)
            {
                LED_OFF;
                enable_wn = true;
                mn_count = 0;
            }
            if (mn_count == mn_num)
            {
                ESP_LOGI(TAG, "stop multinet");
                LED_OFF;
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
static char *weather, string[25] = {0};
static float temp;
static clk_t clk;
static int id = 0;
/*
 * 定时器回调函数
 * 空调定时关闭
 */
timer_cb ir_close_cb(struct timer *tmr, void *arg)
{


    if (id == 0)
    {
        ac_open(false);
        ESP_LOGI(TAG, "timer to close the aircon");
    }
    else
    {
        ac_open(true);
        ESP_LOGI(TAG, "timer to open the aircon");
    }

    return NULL;
}



/*
 * 语音识别处理函数
 */
static esp_err_t asr_multinet_control(int commit_id)
{
    
    if (commit_id >= 0 && commit_id < ID_MAX)
    {
        switch (commit_id)
        {
        case ID0_SHEZHIKONGTIAOERSHIDU:
            ac_set_temp(20);

            ESP_LOGI(TAG, "ID0_SHEZHIKONGTIAOERSHIDU");
            AC_SUCCESS_MP3;
            break;

        case ID1_SHEZHIKONGTIAOERSHIYIDU:
            ac_set_temp(21);

            ESP_LOGI(TAG, "ID1_SHEZHIKONGTIAOERSHIYIDU");
            AC_SUCCESS_MP3;
            break;

        case ID2_SHEZHIKONGTIAOERSHIERDU:
            ac_set_temp(22);

            ESP_LOGI(TAG, "ID2_SHEZHIKONGTIAOERSHIERDU");
            AC_SUCCESS_MP3;
            break;

        case ID3_SHEZHIKONGTIAOERSHISANDU:
            ac_set_temp(23);

            ESP_LOGI(TAG, "ID3_SHEZHIKONGTIAOERSHISANDU");
            AC_SUCCESS_MP3;
            break;

        case ID4_SHEZHIKONGTIAOERSHISIDU:
            ac_set_temp(24);

            ESP_LOGI(TAG, "ID4_SHEZHIKONGTIAOERSHISIDU");
            AC_SUCCESS_MP3;
            break;

        case ID5_SHEZHIKONGTIAOERSHIWUDU:
            ac_set_temp(25);

            ESP_LOGI(TAG, "ID5_SHEZHIKONGTIAOERSHIWUDU");
            AC_SUCCESS_MP3;
            break;

        case ID6_SHEZHIKONGTIAOERSHLIUIDU:
            ac_set_temp(26);
            ESP_LOGI(TAG, "ID6_SHEZHIKONGTIAOERSHLIUIDU");
            AC_SUCCESS_MP3;
            break;

        case ID7_SHEZHIKONGTIAOERSHIQIDU:
            ac_set_temp(27);

            ESP_LOGI(TAG, "ID7_SHEZHIKONGTIAOERSHIQIDU");
            AC_SUCCESS_MP3;
            break;

        case ID8_SHEZHIKONGTIAOERSHIBADU:
            ac_set_temp(28);

            ESP_LOGI(TAG, "ID8_SHEZHIKONGTIAOERSHIBADU");
            AC_SUCCESS_MP3;
            break;

        case ID9_QIDONGKONGTIAOSAOFENG:
            ac_set_swing(true);
            ESP_LOGI(TAG, "ID9_QIDONGKONGTIAOSAOFENG");
            AC_SUCCESS_MP3;
            break;
        case ID10_TINGZHIKONGTIAOSAOFENG:
            ac_set_swing(false);
            ESP_LOGI(TAG, "ID10_TINGZHIKONGTIAOSAOFENG");
            AC_SUCCESS_MP3;
            break;
        case ID11_SHEZHIKONGTIAOZIDONGFENGSU:
            ac_set_wind_speed(0);

            ESP_LOGI(TAG, "ID11_SHEZHIKONGTIAOZIDONGFENGSU");
            AC_SUCCESS_MP3;
            break;

        case ID12_SHEZHIKONGTIAOYIJIFENGSU:
            ac_set_wind_speed(1);
            ESP_LOGI(TAG, "ID12_SHEZHIKONGTIAOYIJIFENGSU");
            AC_SUCCESS_MP3;
            break;

        case ID13_SHEZHIKONGTIAOERJIFENGSU:
            ac_set_wind_speed(2);
            ESP_LOGI(TAG, "ID13_SHEZHIKONGTIAOERJIFENGSU");
            AC_SUCCESS_MP3;
            break;

        case ID14_SHEZHIKONGTIAOSANJIFENGSU:
            ac_set_wind_speed(3);

            ESP_LOGI(TAG, "ID14_SHEZHIKONGTIAOSANJIFENGSU");
            AC_SUCCESS_MP3;
            break;

        case ID15_YIXIAOSHIHOUGUANBIKONGTIAO:
            clk.value = global_clk.value;
            clk.cal.hour += 1;
            goto timer;

            ESP_LOGI(TAG, "ID15_YIXIAOSHIHOUGUANBIKONGTIAO");
            break;

        case ID16_LIANGXIAOSHIHOUGUANBIKONGTIAO:
            clk.value = global_clk.value;
            clk.cal.hour += 2;
            goto timer;

            ESP_LOGI(TAG, "ID16_LIANGXIAOSHIHOUGUANBIKONGTIAO");
            break;

        case ID17_SANXIAOSHIHOUGUANBIKONGTIAO:
            clk.value = global_clk.value;
            clk.cal.hour += 3;
            goto timer;

            ESP_LOGI(TAG, "ID17_SANXIAOSHIHOUGUANBIKONGTIAO");
            break;
        case ID18_SIXIAOSHIHOUGUANBIKONGTIAO:
            clk.value = global_clk.value;
            clk.cal.hour += 4;
            goto timer;

            weather = get_Weather_String(0);
            speech_sync(weather);
            ESP_LOGI(TAG, "ID18_SIXIAOSHIHOUGUANBIKONGTIAO");
            break;
        case ID19_WUXIAOSHIHOUGUANBIKONGTIAO:
            clk.value = global_clk.value;
            clk.cal.hour += 5;
            goto timer;
            ESP_LOGI(TAG, "ID19_WUXIAOSHIHOUGUANBIKONGTIAO");

        case ID20_LIUXIAOSHIHOUGUANBIKONGTIAO:
            clk.value = global_clk.value;
            clk.cal.hour += 6;
            goto timer;
            ESP_LOGI(TAG, "ID20_LIUXIAOSHIHOUGUANBIKONGTIAO");

        case ID21_QIXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID21_QIXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 7;
            goto timer;

        case ID22_BAXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID22_BAXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 8;
            goto timer;

        case ID23_JIUXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID23_JIUXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 9;
            goto timer;

        case ID24_SHIXIAOSHIHOUGUANBIKONGTIAO:

            ESP_LOGI(TAG, "ID24_SHIXIAOSHIHOUGUANBIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.hour += 10;
            goto timer;

        case ID25_DAKAIKONGTIAO:
            ESP_LOGI(TAG, "ID25_DAKAIKONGTIAO");
            ac_open(true);
            AC_SUCCESS_MP3;
            break;

        case ID26_GUANBIKONGTIAO:
            ac_open(false);
            AC_CLOSE_MP3;
            ESP_LOGI(TAG, "ID26_GUANBIKONGTIAO");
            break;

            //////////////////////////////////
        case ID30_DAKAILANYA:

            ESP_LOGI(TAG, "ID30_DAKAILANYA");
            break;

        case ID31_GUANBILANYA:

            ESP_LOGI(TAG, "ID31_GUANBILANYA");
            break;

        case ID32_MINGTIANTIANQIZENMEYANG:

            ESP_LOGI(TAG, "ID32_MINGTIANTIANQIZENMEYANG");
            weather = get_Weather_String(1);
            speech_sync(weather);
            break;
        case ID33_JINTIANTIANQIZENMEYANG:

            ESP_LOGI(TAG, "ID33_JINTIANTIANQIZENMEYANG");
            weather = get_Weather_String(0);
            speech_sync(weather);
            break;
        case ID34_HOUTIANTIANQIZENMEYANG:
            ESP_LOGI(TAG, "ID34_HOUTIANTIANQIZENMEYANG ");
            weather = get_Weather_String(2);
            speech_sync(weather);
            break;

        case ID35_SHINEIWENDU:
            ESP_LOGI(TAG, "ID35_SHINEIWENDU");
            temp = ds18b20_get_data();

            sprintf(string, "当前室温 %.1f 度", temp);
            speech_sync(string);
            break;

        case ID36_XIANZAIJIDIAN:
            ESP_LOGI(TAG, "ID36_XIANZAIJIDIAN");
            sprintf(string, "20%d-%d-%d %d:%d:%d\r\n", global_clk.cal.year, global_clk.cal.month, global_clk.cal.date, global_clk.cal.hour, global_clk.cal.minute, global_clk.cal.second);
            speech_sync(string);

            break;
        case ID37_HONGWAIXUEXI:
            ESP_LOGI(TAG, "ID37_HONGWAIXUEXI");
            ir_study();
            break;
        case ID40_SHIMIAOHOUGUANBIKONGTIAO:
            ESP_LOGI(TAG, "ID40_SHIMIAOHOUGUANBIKONGTIAO:%s", string);
            clk.value = global_clk.value;
            clk.cal.second += 10;
            id = 0;
            tmr_new(&clk, ir_close_cb, &id, "10s");
            SETTMR_MP3;

            break;
        case ID41_JIUMIAOHOUDAKAIKONGTIAO:
            ESP_LOGI(TAG, "ID41_WUMIAOHOUDAKAIKONGTIAO");
            clk.value = global_clk.value;
            clk.cal.second += 9;
            id = 1;
            tmr_new(&clk, ir_close_cb, &id, "5s");
            SETTMR_MP3;

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
