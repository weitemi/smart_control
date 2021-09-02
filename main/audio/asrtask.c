
#include "asrtask.h"
#include "cJSON.h"
#include "myhttp.h"
#include "ir.h"
#include "player.h"
#include "etymology.h"
#include "audio_pipeline.h"
#include "esp_system.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "i2s_stream.h"
#include "raw_stream.h"
#include "tone_stream.h"
#include "audio_tone_uri.h"
#include "mp3_decoder.h"
#include "filter_resample.h"
#include "rec_eng_helper.h"
#include "esp_vad.h"
#include "myble.h"

#define VAD_SAMPLE_RATE_HZ 16000
#define VAD_FRAME_LENGTH_MS 30
#define VAD_BUFFER_LENGTH (VAD_FRAME_LENGTH_MS * VAD_SAMPLE_RATE_HZ / 1000) //480

#define RECODER_TIMEOUT 25 //(25*960)/32k=23.4k/32k = 0.7s

#define BAIDU_ASR_URL "https://vop.baidu.com/pro_api?dev_pid=80001&cuid=esp32&token=%s"

#define MAX_RECODER (128 * 1024) //4*32k=128k=4s
#define MAX_HTTP_LEN (2048)
#define POST_DATA_LEN (50)

static void asr_control();
static char url[200];

static const char *TAG = "asrtask.c";
static char http_buff[MAX_HTTP_LEN] = {0}; //语音使用的http缓存
static char *recoder = NULL; //录音buff


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

    ESP_LOGI(TAG, "start baiduasr");
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
    order_t ord = etymology(result);
    cJSON_Delete(root);
}


void ASR_Task(void *agr)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    esp_wn_iface_t *wakenet; //语音唤醒模型
    model_coeff_getter_t *model_coeff_getter;
    model_iface_data_t *model_wn_data; //模型接口数据

    get_wakenet_iface(&wakenet);                                      //获取唤醒模型接口
    get_wakenet_coeff(&model_coeff_getter);                           //获取唤醒模型系数
    model_wn_data = wakenet->create(model_coeff_getter, DET_MODE_90); //创建唤醒模型。检查率90%

    int audio_wn_chunksize = wakenet->get_samp_chunksize(model_wn_data); //获取唤醒模型的采样音频的大小

    int16_t *buffer = (int16_t *)malloc(audio_wn_chunksize * sizeof(short)); //用来缓存采样的音频

    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_reader, filter, raw_read;
    bool enable_wn = true;

    //创建语音唤醒管道流
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

    vad_handle_t vad_inst = vad_create(VAD_MODE_4, VAD_SAMPLE_RATE_HZ, VAD_FRAME_LENGTH_MS); //创建音量检测模型

    int16_t *vad_buff = (int16_t *)malloc(VAD_BUFFER_LENGTH * sizeof(short)); //录音buffer
    if (vad_buff == NULL)
    {
        ESP_LOGE(TAG, "Memory allocation failed!");
    }
    while (1)
    {
        //读取管道的音频缓存到buffer 960k
        raw_stream_read(raw_read, (char *)buffer, audio_wn_chunksize * sizeof(short)); //960
        if (enable_wn)
        {
            //将音频数据输入模型
            if (wakenet->detect(model_wn_data, (int16_t *)buffer) == 1)
            {
                ESP_LOGI(TAG, "wake up start listening");
                //匹配，唤醒
                enable_wn = false;
            }
        }
        else
        {
            //TODO 离线在线混合使用？
            static int index = 0;
            static int total = 0; //录音时间
            static int timeout = 0; //超过一定时间无声音则停止录音
            //唤醒后，raw_stream_read继续读取音频到buffer
            if (recoder != NULL)
            {
                //判断 达到录音最长或停止说话
                if (total < (MAX_RECODER - 960) && timeout < RECODER_TIMEOUT)
                {
                    //继续录音
                    //将buffer的音频复制到recoder
                    memcpy(recoder + (index * audio_wn_chunksize * sizeof(short)), buffer, audio_wn_chunksize * sizeof(short));
                    index++;
                    //记录总音频数据大小
                    total += audio_wn_chunksize * sizeof(short); //max=131072
                }
                else
                {
                    //停止录音 准备将音频发送到百度api
                    ESP_LOGI(TAG, "stop listening");
                    memset(http_buff, 0, MAX_HTTP_LEN); //重置http buff
                    baidu_asr((const char *)recoder, total);
                    free(recoder); //释放录音内存
                    recoder = NULL;
                    index = total = timeout = 0;
                    enable_wn = true; //进入睡眠，等下次唤醒

                }
            }
            else    /*if(recoder!=NULL)*/
            {
                recoder = malloc(MAX_RECODER); //为录音分配内存
                if(recoder==NULL)
                {
                    enable_wn = true;
                    ESP_LOGI(TAG, "fail to malloc for recoder");
                }
            }

            //复制buffer的音频数据到vad_buff
            memcpy(vad_buff, buffer, VAD_BUFFER_LENGTH * sizeof(short));

            //将vad_buff的音频输入到声音检测模型
            vad_state_t vad_state = vad_process(vad_inst, vad_buff);

            //判断是否有声音
            if (vad_state == VAD_SPEECH)
            {
                //讲话未结束
                timeout = 0;
            }
            else
            {
                //计时
                timeout++;
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
        ord.number = ord.number > 0 ? ord.number : 26;
        if(ord.action)
            ac_set_temp(ord.number);
        else
            ac_open(false);
        break;
    case 2:
        if(ord.action)
            ble_open();
        else
            ble_close();
        break;
    case 3:
        ESP_LOGI(TAG,"%s", get_Weather_String(ord.time.day));
        break;
    default:
        break;
    }
}