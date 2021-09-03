
#include "asrtask.h"
#include "cJSON.h"
#include "myhttp.h"
#include "ir.h"
#include "player.h"
#include "etymology.h"
#include "myble.h"
#include "general_gpio.h"

#include "audio_pipeline.h"
#include "esp_system.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "i2s_stream.h"
#include "raw_stream.h"
#include "audio_tone_uri.h"
#include "filter_resample.h"
#include "rec_eng_helper.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"

#define BAIDU_ASR_URL "https://vop.baidu.com/pro_api?dev_pid=80001&cuid=esp32&token=%s"

//960是一个样本大小
#define MAX_RECODER (960 * 135)
#define MAX_HTTP_LEN (2048)

static void asr_control();
static esp_err_t asr_multinet_control(int commit_id);
static char url[200];

static const char *TAG = "asrtask.c";
EXT_RAM_ATTR static char http_buff[MAX_HTTP_LEN]; //语音使用的http缓存
//外部RAM
EXT_RAM_ATTR char recoder[MAX_RECODER]; //录音buff

typedef enum
{
    ID0 = 0,
    ID1 = 1,
    ID2 = 2,
    ID3 = 3,
    ID4 = 4,
    ID5 = 5,
    ID6 = 6,
    ID7 = 7,
    ID8 = 8,
    ID9 = 9,
    ID_MAX,
} asr_multinet_event_t;
/*
 brief:http连接到百度asr
*/
static void baidu_asr(const char *data, int len)
{
    //配置http_client
    esp_http_client_config_t config = {
        .method = HTTP_METHOD_POST,         //post方式
        .event_handler = http_event_handle, //注册http回调函数
        .user_data = (void *)http_buff,     //传递参数
    };
    memset(url, 0, 200);
    sprintf(url, BAIDU_ASR_URL, baidu_access_token); //将token组装到url
    config.url = url;
    //printf("start connect to url = %s\r\n", config.url);
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "audio/pcm;rate=16000"); //设置http头部
    esp_http_client_set_post_field(client, data, len);                          //将录音添加到http body

    ESP_LOGI(TAG, "start baidu asr");
    esp_http_client_perform(client); //执行http连接
    esp_http_client_close(client);   //关闭清除 等待回调函数
    esp_http_client_cleanup(client);

    //cjson解析asr结果
    cJSON *root = cJSON_Parse(http_buff);
    if (root == NULL)
    {
        ESP_LOGI(TAG, "cjson parse error");
    }
    cJSON *item = cJSON_GetObjectItem(root, "err_no");
    if (item->valueint != 0)
    {
        ESP_LOGI(TAG, "translate error,err_no:%d", item->valueint);
        cJSON_Delete(root);
    }
    item = cJSON_GetObjectItem(root, "result");
    item = cJSON_GetArrayItem(item, 0);
    char *result = cJSON_GetStringValue(item); //获取语音识别文本
    ESP_LOGI(TAG, "baidu_asr result:%s", result);
    order_t ord = etymology(result);
    cJSON_Delete(root);
    ESP_LOGI(TAG, "day=%d sig=%d hour=%d,min=%d obj=%d action=%d number=%d", ord.time.day, ord.time.sig, ord.time.hour, ord.time.min, ord.obj, ord.action, ord.number);
    asr_control(ord);
}

void ASR_Task(void *agr)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    esp_wn_iface_t *wakenet;
    model_coeff_getter_t *model_coeff_getter;
    model_iface_data_t *model_wn_data; //模型接口数据
    const esp_mn_iface_t *multinet = &MULTINET_MODEL;

    /* 创建语音唤醒模型 */
    get_wakenet_iface(&wakenet);                                         //获取唤醒模型接口
    get_wakenet_coeff(&model_coeff_getter);                              //获取唤醒模型系数
    model_wn_data = wakenet->create(model_coeff_getter, DET_MODE_90);    //创建唤醒模型。检查率90%
    int audio_wn_chunksize = wakenet->get_samp_chunksize(model_wn_data); //获取唤醒模型的采样音频的大小

    /* 创建语音识别模型 */
    model_iface_data_t *model_mn_data = multinet->create(&MULTINET_COEFF, 4000); //语音识别时间，single模式下最大4s
    int audio_mn_chunksize = multinet->get_samp_chunksize(model_mn_data);        //识别内存块
    int mn_num = multinet->get_samp_chunknum(model_mn_data);

    /* 创建采样音频样本点缓存buffer */
    int size = audio_wn_chunksize;
    if (audio_mn_chunksize > audio_wn_chunksize)
    {
        size = audio_mn_chunksize;
    }
    int sample_buffer_size = size * sizeof(short);           //应该是960byte
    int16_t *buffer = (int16_t *)malloc(sample_buffer_size); //用来缓存采样的音频?大小？
    ESP_LOGI(TAG, "sample_buffer_size = %d bytes,need recoder=%d bytes", sample_buffer_size, mn_num * sample_buffer_size);

    /* 创建音频流 */
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_reader, filter, raw_read;
    bool enable_wn = true;
    uint32_t mn_count = 0;

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.i2s_config.sample_rate = 48000;        //i2s采样率
    i2s_cfg.type = AUDIO_STREAM_READER;            //i2s为输入
    i2s_stream_reader = i2s_stream_init(&i2s_cfg); //i2s流初始化

    //重采样滤波器设置
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = 48000;           //重采样率
    rsp_cfg.src_ch = 2;                 //输入音频为双声道
    rsp_cfg.dest_rate = 16000;          //滤波后的音频的采样率
    rsp_cfg.dest_ch = 1;                //滤波后单声道
    filter = rsp_filter_init(&rsp_cfg); //滤波器流初始化

    //raw流初始化
    raw_stream_cfg_t raw_cfg = {
        .out_rb_size = 8 * 1024,     //输出buffer大小8k
        .type = AUDIO_STREAM_READER, //输入流
    };
    raw_read = raw_stream_init(&raw_cfg); //初始化raw流

    //将三个流注册到管道
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s");
    audio_pipeline_register(pipeline, raw_read, "raw");
    audio_pipeline_register(pipeline, filter, "filter");

    ESP_LOGI(TAG, "[ 4 ] Link elements together [codec_chip]-->i2s_stream-->filter-->raw-->[SR]");
    const char *link_tag[3] = {"i2s", "filter", "raw"};
    //将个音频流连接
    audio_pipeline_link(pipeline, &link_tag[0], 3);

    audio_pipeline_run(pipeline); //运行管道

    ESP_LOGI(TAG, "ASR Init OK");
    while (1)
    {
        static int index = 0;
        static int total = 0;

        //读取管道的音频缓存到buffer 960k
        raw_stream_read(raw_read, (char *)buffer, sample_buffer_size); //960
        if (enable_wn)
        {
            /* 语音唤醒 */
            if (wakenet->detect(model_wn_data, (int16_t *)buffer) == 1)
            {
                LED_ON;
                ESP_LOGI(TAG, "wake up");
                enable_wn = false;
                //清除缓存，准备录音
                memset(recoder, 0, total);
                index = total = 0;
            }
        }
        else
        {
            /* 离线语音识别 */
            mn_count++;
            int commit_id = multinet->detect(model_mn_data, buffer);

            if (asr_multinet_control(commit_id) == ESP_OK)
            {
                LED_OFF;
                enable_wn = true;
                mn_count = 0;
            }

            if (total < (MAX_RECODER - 960))
            {
                //将buffer的音频复制到recoder
                memcpy(recoder + (index * sample_buffer_size), buffer, sample_buffer_size);
                index++;
                total += sample_buffer_size; //max=131072
            }
            else
            {
                //理想情况是不会超出内存，因为mn_num的限制
                ESP_LOGI(TAG, "recoder overflow!");
                enable_wn = true;
                mn_count = 0;
                LED_OFF;
            }

            /* 本地语音识别未能完成识别，交给语音在线识别*/
            if (mn_count == mn_num)
            {
                ESP_LOGI(TAG, "stop listening");
                LED_OFF;
                memset(http_buff, 0, MAX_HTTP_LEN);
                baidu_asr((const char *)recoder, total);
                enable_wn = true;
                mn_count = 0;
            }
        }
    }
    //关闭管道
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
    wakenet->destroy(model_wn_data); //销毁模型
    model_wn_data = NULL;
    free(buffer); //释放内存
    buffer = NULL;
}

void ASR_Init(void)
{
    xTaskCreate(ASR_Task, "ASR_Task", 4096, NULL, 5, NULL);
}

//asr执行代码
void asr_control(order_t ord)
{
    //TODO 加入定时代码
    switch (ord.obj)
    {
    case 1:
    {
        ord.number = ord.number > 0 ? ord.number : 26;
        if (ord.action)
        {
            ESP_LOGI(TAG, "order set aircon tmp=%d", ord.number);
            ac_set_temp(ord.number);
        }
        else
        {
            ESP_LOGI(TAG, "order close aircon");
            ac_open(false);
        }
        break;
    }
    case 2:
    {
        if (ord.action)
        {
            ESP_LOGI(TAG, "order open ble");
            ble_open();
        }
        else
        {
            ESP_LOGI(TAG, "order close ble");
            ble_close();
        }
        break;
    }
    case 3:
    {
        speech_sync(get_Weather_String(ord.time.day));
        break;
    }

    default:
        break;
    }
}
static esp_err_t asr_multinet_control(int commit_id)
{
    if (commit_id >= 0 && commit_id < ID_MAX)
    {
        switch (commit_id)
        {
        case ID0:
            ac_open(true);
            /* code */
            break;
        case ID1:
            ac_open(false);
            /* code */
            break;
        case ID2:
            ac_set_wind_speed(ac_get_wind_speed() + 1);
            /* code */
            break;
        case ID3:
            ac_set_wind_speed(ac_get_wind_speed() - 1);
            /* code */
            break;
        case ID4:
            ac_set_temp(ac_get_temp()+1);
            /* code */
            break;
        case ID5:
            ac_set_temp(ac_get_temp()-1);
            /* code */
            break;
        case ID6:
            
            /* code */
            break;
        case ID7:
            /* code */
            break;
        case ID8:
            ble_open();
            /* code */
            break;
        case ID9:
            ble_close();
            /* code */
            break;
        default:
            break;
        }
        return ESP_OK;
    }
    return ESP_FAIL;
}