#include "player.h"
#include "myhttp.h"
#include "mywifi.h"
#include "esp_http_client.h"
#include "esp_audio.h"
#include "audio_pipeline.h"
#include "i2s_stream.h"
#include "http_stream.h"
#include "tone_stream.h"
#include "mp3_decoder.h"
#include "filter_resample.h"

#define BAIDU_TTS_URL "http://tsn.baidu.com/text2audio"
static const char *TAG = "PLAYER";


static char request_data[1024];
esp_audio_handle_t player;

//! 数组不能太小
#define MAX_TEXT_LEN 1024
char text[MAX_TEXT_LEN] = "百度语音合成";



static char *mySTRCPY(char *strDest,  char *strSrc)
{
    char *strDestCopy = strDest; 
    while ((*strDest++ = *strSrc++) != '\0')
        ; 
    return strDestCopy;
}

/*
 * http stream回调函数
 * 调用esp_audio_sync_play(player,BAIDU_TTS_URL, 0)会调用此回调函数
 */
int _http_stream_event_handle(http_stream_event_msg_t *msg)
{
    esp_http_client_handle_t http_client = (esp_http_client_handle_t)msg->http_client;  //获取http客户端
    switch(msg->event_id)
    {
        case HTTP_STREAM_PRE_REQUEST:

            //准备请求百度文字转语音的音频
            //ESP_LOGI(TAG, "HTTP_STREAM_PRE_REQUEST");
            //检查token
            if (baidu_access_token == NULL) {
                // Must freed `baidu_access_token` after used 获得token
                ESP_LOGI(TAG, "try to get token");
                //baidu_access_token = baidu_get_access_token(BAIDU_ACCESS_KEY, BAIDU_SECRET_KEY);
            }
            if (baidu_access_token == NULL) {
                ESP_LOGE(TAG, "Error issuing access token");
                return ESP_FAIL;
            }
            /* 组装body参数：lan：语言；cuid：设备id；ctp：...;aue：文件格式;spd:语速;pit：音调；vol：音量；per：人声*/
            int data_len = snprintf(request_data, 1024, "lan=zh&cuid=ppp&ctp=2&aue=3&spd=4&pit=5&vol=5&per=0&tok=%s&tex=%s", baidu_access_token, text);
            //将token装填进http请求
            esp_http_client_set_post_field(http_client, request_data, data_len);
            //post
            esp_http_client_set_method(http_client, 1);
            break;

        case HTTP_STREAM_ON_REQUEST:
            //ESP_LOGI(TAG, "HTTP_STREAM_ON_REQUEST");
            break;
        case HTTP_STREAM_ON_RESPONSE:
            //ESP_LOGI(TAG, "HTTP_STREAM_ON_RESPONSE");
            break;
        case HTTP_STREAM_POST_REQUEST:
            //ESP_LOGI(TAG, "HTTP_STREAM_POST_REQUEST");
            break;
        case HTTP_STREAM_FINISH_REQUEST:
            //ESP_LOGI(TAG, "HTTP_STREAM_FINISH_REQUEST");
            break;    
        case HTTP_STREAM_RESOLVE_ALL_TRACKS:
            //ESP_LOGI(TAG, "HTTP_STREAM_RESOLVE_ALL_TRACKS");
            break;
        case HTTP_STREAM_FINISH_TRACK:
            //ESP_LOGI(TAG, "HTTP_STREAM_FINISH_TRACK");
            break;
        case HTTP_STREAM_FINISH_PLAYLIST:
            //ESP_LOGI(TAG, "HTTP_STREAM_FINISH_PLAYLIST");
            break;        
    }
    return ESP_OK;
}

/*
 * 播放器初始化
 * 可播放flash mp3与http音频流
 */
int player_init()
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "setup player");

    /*板子音频芯片初始化 音频引脚初始化 更换芯片或者板子时要注意*/
    esp_audio_cfg_t cfg = DEFAULT_ESP_AUDIO_CONFIG();   //默认音频配置
    ESP_LOGI(TAG, "audio_board_init");
    audio_board_handle_t board_handle = audio_board_init(); //音频板子初始化
    cfg.vol_handle = board_handle->audio_hal;
    cfg.prefer_type = ESP_AUDIO_PREFER_MEM;
    
    ESP_LOGI(TAG, "create player");
    //!对于我们的开发板，要使用48k才能正常说话（喇叭）
    cfg.resample_rate = 48000;  //音频重采样率
    player = esp_audio_create(&cfg);    //根据cfg创建player对象

    ESP_LOGI(TAG, "init codec");
    //安装ac101（音频编解码芯片）驱动
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[2.1] Create http stream to read data");
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT(); //配置http音频流

    http_cfg.event_handle = _http_stream_event_handle;    //设置httpstream的回调函数
    http_cfg.type = AUDIO_STREAM_READER;    //read类型的音频流
    audio_element_handle_t http_stream_reader = http_stream_init(&http_cfg);    //创建http流
    esp_audio_input_stream_add(player, http_stream_reader);   //将http输入流添加到player的输入源
    
    ESP_LOGI(TAG, "create flash tone stream reader");
    tone_stream_cfg_t tone_cfg = TONE_STREAM_CFG_DEFAULT(); //配置flash_tone音频流
    tone_cfg.type = AUDIO_STREAM_READER;    //read类型的音频流
    audio_element_handle_t flash_tone_stream_reader = tone_stream_init(&tone_cfg);  //创建flash tone流
    ESP_LOGI(TAG, "add stream reader to player");
    esp_audio_input_stream_add(player, flash_tone_stream_reader); //将flash tone输入流添加到player输入源

    ESP_LOGI(TAG, "create mp3 decoder and add to player");
    mp3_decoder_cfg_t  mp3_dec_cfg  = DEFAULT_MP3_DECODER_CONFIG(); //配置mp3解码器
    audio_element_handle_t mp3_decoder = mp3_decoder_init(&mp3_dec_cfg);    //创建mp3解码器
    esp_audio_codec_lib_add(player, AUDIO_CODEC_TYPE_DECODER,mp3_decoder ); //将mp3解码器添加进player的解码库

    
    ESP_LOGI(TAG, "create i2s writer and add to player");
    i2s_stream_cfg_t i2s_writer = I2S_STREAM_CFG_DEFAULT();
    //!i2s采样率 注意要和player的采样率48000相同
    i2s_writer.i2s_config.sample_rate = 48000; 
    
    i2s_writer.i2s_config.channel_format = I2S_CHANNEL_FMT_ALL_RIGHT;   //i2s输出右声道
    i2s_writer.type = AUDIO_STREAM_WRITER;  //writer类型的音频流
    audio_element_handle_t i2s_stream_writer = i2s_stream_init(&i2s_writer);    //创建i2s流
    esp_audio_output_stream_add(player, i2s_stream_writer); //i2s writer添加到player的输出源

    //ESP_LOGI(TAG, "http_stream_reader->mp3decode->player->i2s->ac101");
    //ESP_LOGI(TAG, "flash_stream_reader->mp3decode->player->i2s->ac101");

    esp_audio_vol_set(player, 80);  //设置player的音量

    ESP_LOGI(TAG, "Player Init OK");
    return ESP_OK;
}

/*
 * 播放flash中的mp3
 * index：audio_tone_uri.h中定义的mp3文件路径下标
 */
int play_flash(const tone_type_t index)
{
    ESP_LOGI(TAG, "play flash :%s", tone_uri[index]);
    return esp_audio_sync_play(player,tone_uri[index], 0);
 
}

/*
 * 播放百度语音合成
 * t:中文字符串指针
 */
int speech_sync(char* t)
{
    //检查wifi连接
    if(!get_wifi_status())
    {
        ESP_LOGI(TAG, "wifi disconnect,can't tts");
        return ESP_FAIL;
    }
    //ESP_LOGI(TAG, "start speech sync t = %s",t);
    mySTRCPY(text, t);  //复制文本，准备http请求
    ESP_LOGI(TAG, "start speech sync text = %s",text);
    return esp_audio_sync_play(player,BAIDU_TTS_URL, 0);
}

void player_testTask(void *arg)
{
    while(1)
    {
        ESP_LOGI(TAG, "player_testTask ");
        if(get_wifi_status())
        {
            char *str = "百度语音合成测试";
            speech_sync(str);
        }else{
            //play_flash(TONE_TYPE_AIR_COND_CLOSE);
        }
        vTaskDelay(1000);
    }
}




