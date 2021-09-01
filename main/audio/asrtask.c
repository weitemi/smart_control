
#include "asrtask.h"
#include "cJSON.h"
#include "myhttp.h"
#include "ir.h"
#include "player.h"

#define USE_ETY 1
#define VAD_SAMPLE_RATE_HZ 16000
#define VAD_FRAME_LENGTH_MS 30
#define VAD_BUFFER_LENGTH (VAD_FRAME_LENGTH_MS * VAD_SAMPLE_RATE_HZ / 1000) //480

#define RECODER_TIMEOUT 25   //(25*960)/32k=23.4k/32k = 0.7s


#define BAIDU_ASR_URL "https://vop.baidu.com/pro_api?dev_pid=80001&cuid=esp32&token=%s"
#define BAIDU_ETY_URL "https://aip.baidubce.com/rpc/2.0/nlp/v1/lexer_custom?charset=UTF-8&access_token=%s"


#define MAX_RECODER (128*1024)  //4*32k=128k=4s
#define MAX_HTTP_LEN (2048)
#define POST_DATA_LEN (50)

static char url[200];
//对空调的操作
enum AC_Option
{
    AC_OPEN = 0,    //开
    AC_CLOSE,   //关
    AC_UP,  //增加
    AC_DOWN,    //减少
    AC_OTHER    
};

//语音可操作的对象
enum Object
{
    obj_Ac = 0, //空调
    obj_Bt, //蓝牙
    obj_Weather,    //天气
    obj_other
};

//词性
enum Lexical    
{
    Verbs = 0,  //动词
    Nouns,  //名词
    Mount,  //量词
    Word,   //字
    Pronouns,   //代词

    //以下是定制的特征词
    Open,   
    Close,
    Up,
    Down,
    Num,    //数字
    TIME,   //时间
    Aircon,
    Bt,
    Weather,
    Today,
    Tomorrow,
    Aftermotorrow,
    Other
};

//词性解析出的元素如：打开
typedef struct 
{
    enum Lexical lexical;   //词性或特征
    char text[10];  //文本内容
}Ety_Element;
static Ety_Element ety_eles[10] = {0};

//命令结构体包括命令的对象，操作，数量，时间（未完成）
typedef struct{
    int number;
    enum Object object;    //only for "Aircon Bt Weather"
    enum AC_Option option;    //only for "open close up down "
    
} Audio_Order;


static const char *TAG = "asrtask.c";
TaskHandle_t ASR_task_handle;
static char http_buff[MAX_HTTP_LEN] = {0};  //语音使用的http缓存

char *recoder=NULL; //录音buff

static int Etymology_Analysis( );
static int parse_items();
static Audio_Order build_order(int i);

static int ac_order(enum AC_Option opt,int temp);
static int bt_order(enum AC_Option opt);

int bt_order(enum AC_Option opt)
{
    if(opt==AC_OPEN)
    {
        //蓝牙开启函数
        ESP_LOGI(TAG, "BT OPEN");
        
    }
    else if(opt==AC_CLOSE)
    {
        //关闭蓝牙函数
        ESP_LOGI(TAG, "BT CLOSE");
        
    }
    else
        return 0;
    return 1;
}

/*
 * 语音发送空调的命令
 * opt：对空调的操作
 * temp：温度变化量
 */
int ac_order(enum AC_Option opt,int temp)
{
    ESP_LOGI(TAG, "temp =%d",temp);

    switch (opt)
    {
    case AC_OPEN:
        ac_open(true);
        ESP_LOGI(TAG, "AC_OPEN");
        break;
    case AC_CLOSE:
        ac_open(false);
        ESP_LOGI(TAG, "AC_CLOSE");
        break;

    case AC_DOWN:
        ESP_LOGI(TAG, "AC_DOWN");
        break;

    case AC_UP:
        ESP_LOGI(TAG, "AC_UP");
        break;
    default:
        ESP_LOGI(TAG, "option error");
        return -1;
    }

    //限制温度值
    if(temp<AC_TEMP_16||temp>AC_TEMP_30)
    {
        ESP_LOGI(TAG, "temp err");
        return 0;
    }else
    {
        
    }

    //发送红外载波
    

    //语音播放

    
    //返回温度数据
    return 1;
}
//-1:无效；可表示今天、明天、后天；数字
static int number;
/*
 * 组装一个语音命令
 * i:命令中的单词数量
 * 
 */
Audio_Order build_order(int i)
{
    //初始化一个语音命令
    Audio_Order ord={
        .number=0,
        .object=obj_other,
        .option=AC_OTHER
    };
    ord.number = number;
    //遍历单词，提取对命令有关的信息
    for (int x = 0; x < i; x++)
    {
        //寻找操作对象
        switch(ety_eles[x].lexical)
        {
            case Aircon:
                ord.object = obj_Ac;
                break;
            case Bt:
                ord.object = obj_Bt;
                break;
            case Weather:
                ord.object = obj_Weather;
                break;
            case Open:
                ord.option = AC_OPEN;
                break;
            case Close:
                ord.option = AC_CLOSE;
                break;
            case Up:
                ord.option = AC_UP;
                break;
            case Down:
                ord.option = AC_DOWN;
                break;
            case Num:
                //ord.number = atoi(ety_eles[x].text);//字符串数字转整型数字
                //printf("num=%d\r\n", ord.number);
                break;
            case Mount:
                //ord.number = atoi(ety_eles[x].text);
                //printf("num=%d\r\n", ord.number);
                
            case TIME:

                break;
            case Today:
                ord.number = 0;
                break;
            case Tomorrow:
                ord.number = 1;
                break;
            case Aftermotorrow:
                ord.number = 2;
                break;
            //其他属性忽略
            default:
            
                break;
        
        }

    }
   
    return ord;
}
/*
 * 解析每个单词的词性
 * 返回：单词数量
 */
int parse_items()
{
    cJSON *root = cJSON_Parse(http_buff);   //解析语音json

    cJSON *items = cJSON_GetObjectItem(root, "items");
    if(items == NULL)
    {
        return 0;
    }

    int arry_size=cJSON_GetArraySize(items);

    //每个ety_eles存放一个单词 清空准备接收新的单词
    memset(ety_eles, 0, 10 * sizeof(Ety_Element));

    cJSON *item,*sub_item;
    char *character, *text; //词性及文本内容

    for (int i = 0; i < arry_size; i++)
    {
        item = cJSON_GetArrayItem(items, i);
        //ne和pos都是描述词性，两者只能出现一个
        sub_item = cJSON_GetObjectItem(item, "pos");

        character = cJSON_GetStringValue(sub_item);

        //pos为空串时说明ne有效
        if (strncmp(character,"",1)==0)
        {
            //ESP_LOGI(TAG, "pos is null");
            sub_item = cJSON_GetObjectItem(item, "ne");//!ne需要特殊处理
            character = cJSON_GetStringValue(sub_item);
        }
        
        //sub_item = cJSON_GetObjectItem(item, "item");
        printf("character = %s \r\n", character);
        
        //获取单词的词性
        if (strncmp(character, "NUM", 3) == 0)
        {
            /*
            ety_eles[i].lexical = Num;
            sub_item = cJSON_GetObjectItem(item, "item");
            text = cJSON_GetStringValue(sub_item);
            strncpy(ety_eles[i].text, text, strlen(text));  //保存二位数字
            */
        }
        else if(strncmp(character,"AC",2)==0){
            ety_eles[i].lexical = Aircon;
        }
        else if(strncmp(character,"BT",2)==0){
            ety_eles[i].lexical = Bt;
        }
        else if(strncmp(character,"WEA",3)==0){
            ety_eles[i].lexical = Weather;
        }
        else if(strncmp(character,"DOWN",4)==0){
            ety_eles[i].lexical = Down;
        }
        else if(strncmp(character,"UP",2)==0){
            ety_eles[i].lexical = Up;
        }
        else if(strncmp(character,"CLOSE",5)==0){
            ety_eles[i].lexical = Close;
        }
        else if(strncmp(character,"OPEN",4)==0){
            ety_eles[i].lexical = Open;
        }
        else if(strncmp(character,"TOMO",4)==0)
        {
            ety_eles[i].lexical = Tomorrow;
        }
        else if(strncmp(character,"AFTTO",5)==0)
        {
            ety_eles[i].lexical = Aftermotorrow;
        }        
        else if(strncmp(character,"TODAY",4)==0)
        {
            ety_eles[i].lexical = Today;
        }
        else if(strncmp(character,"TIME",4)==0){
            //TODO 如何解析中文的时间，暂时不做时间方面的功能
            ety_eles[i].lexical = TIME; 
        }
        else if(strncmp(character,"n",1)==0){
            ety_eles[i].lexical = Nouns;
        }
        else if(strncmp(character,"w",1)==0){
            ety_eles[i].lexical = Word;
        }
        else if(strncmp(character,"v",1)==0){
            ety_eles[i].lexical = Verbs;
        }
        else if(strncmp(character,"m",1)==0){
            //eg：26度 100块 需要提取basiword的第一个
            sub_item = cJSON_GetObjectItem(item, "basic_words");
            sub_item = cJSON_GetArrayItem(sub_item, 0);
            text = cJSON_GetStringValue(sub_item);    //数字字符串
            ety_eles[i].lexical = Mount;
            strncpy(ety_eles[i].text, text, strlen(text));  //保存数量
        }
        else if(strncmp(character,"r",1)==0){
            ety_eles[i].lexical = Pronouns;
        }
        else{
            ety_eles[i].lexical = Other;
        }
        
        //printf("ele char =%u,text=%s \r\n", ety_eles[i].lexical, ety_eles[i].text);

    }
    cJSON_Delete(root);
    
    return arry_size;
}

/*
 * 根据语音结果进行词性分析 是否能用本地实现词性分析
 * 成功返回1
 */
int Etymology_Analysis()
{
    number = -1;
    //获取语音识别的文本结果
    cJSON *root = cJSON_Parse(http_buff);
    if(root==NULL)
    {
        ESP_LOGI(TAG,"cjson parse error");
        return 0;
    }
    cJSON *item=cJSON_GetObjectItem(root, "err_no");
    if(item->valueint!=0)
    {
        ESP_LOGI(TAG,"translate error,err_no:%d",item->valueint);
        cJSON_Delete(root);
        return 0;
    }
    item = cJSON_GetObjectItem(root, "result");
    item = cJSON_GetArrayItem(item,0);
    char *result = cJSON_GetStringValue(item);  //获取语音识别文本

    //ESP_LOGI(TAG, "result=:%s", result);
    //获取文本中的数字
    char *p = result;
    char num_str[5] = {0};
    char *p1 = num_str;
    while (*p!='\0')
    {
        if(*p>47 && *p<58 && p1<p1+5)
        {
            *p1 = *p;
            p1++;
        }
        p++;
    }
    if(num_str[0]!=0)number = atoi(num_str);
    ESP_LOGI(TAG, "number=:%d", number);   

    //将文本添加进json字符串
    char *post_data = malloc(POST_DATA_LEN);

    //将rersult组成json并存到post_data 
    snprintf(post_data, POST_DATA_LEN, "{\"text\":\"%s\"}", result);

    ESP_LOGI(TAG, "POST DATA:%s", post_data);

    //清空http buff准备接收数据
    memset(http_buff, 0, MAX_HTTP_LEN);

    //初始化http客户端 准备调用词性分析api
    esp_http_client_config_t config={
        .method=HTTP_METHOD_POST,   //post方式
        .event_handler=http_event_handle,   //注册回调函数

        .user_data = (void *)http_buff, //传递参数
    };
    memset(url, 0, 200);
    sprintf(url, BAIDU_ETY_URL, baidu_access_token);    //将token加入url
    config.url = url;
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json"); //设置http头
    esp_http_client_set_post_field(client,(const char*)post_data,strlen(post_data));    //将json字符串填入body

    printf("start connect to url = %s\r\n",config.url);
    esp_http_client_perform(client);    //开始连接
    int con_len = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "Status = %d, content_length = %d", esp_http_client_get_status_code(client), con_len);

    //关掉http客户端
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    //删除cjson
    cJSON_Delete(root);
    free(post_data);    //释放postdata，留给下次
    return 1;
}

void ASR_Task(void *agr)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    esp_wn_iface_t *wakenet;    //语音唤醒模型
    model_coeff_getter_t *model_coeff_getter;
    model_iface_data_t *model_wn_data;  //模型接口数据

    get_wakenet_iface(&wakenet);//获取唤醒模型接口
    get_wakenet_coeff(&model_coeff_getter); //获取唤醒模型系数
    model_wn_data = wakenet->create(model_coeff_getter, DET_MODE_90);   //创建唤醒模型。检查率90%

    int audio_wn_chunksize = wakenet->get_samp_chunksize(model_wn_data);    //获取唤醒模型的采样音频的大小

    int16_t *buffer = (int16_t *)malloc(audio_wn_chunksize * sizeof(short));    //用来缓存采样的音频

    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_reader, filter, raw_read;
    bool enable_wn = true;

    //创建语音唤醒管道流
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.i2s_config.sample_rate = 48000; //i2s采样率
    i2s_cfg.type = AUDIO_STREAM_READER; //i2s为输入
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);  //i2s流初始化
    
    //重采样滤波器设置
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = 48000;   //重采样率
    rsp_cfg.src_ch = 2; //输入音频为双声道
    rsp_cfg.dest_rate = 16000;  //滤波后的音频的采样率
    rsp_cfg.dest_ch = 1;    //滤波后单声道
    filter = rsp_filter_init(&rsp_cfg); //滤波器流初始化

    //raw流初始化
    raw_stream_cfg_t raw_cfg = {
        .out_rb_size = 8 * 1024,    //输出buffer大小8k
        .type = AUDIO_STREAM_READER,    //输入流
    };
    raw_read = raw_stream_init(&raw_cfg);   //初始化raw流

    //将三个流注册到管道
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s");
    audio_pipeline_register(pipeline, raw_read, "raw");               
    audio_pipeline_register(pipeline, filter, "filter");

    ESP_LOGI(TAG, "[ 4 ] Link elements together [codec_chip]-->i2s_stream-->filter-->raw-->[SR]");
    const char *link_tag[3] = {"i2s", "filter", "raw"};
    //将个音频流连接
    audio_pipeline_link(pipeline, &link_tag[0], 3);    

    audio_pipeline_run(pipeline);   //运行管道

    vad_handle_t vad_inst = vad_create(VAD_MODE_4, VAD_SAMPLE_RATE_HZ, VAD_FRAME_LENGTH_MS);    //创建音量检测模型

    int16_t *vad_buff = (int16_t *)malloc(VAD_BUFFER_LENGTH * sizeof(short));   //录音buffer
    if (vad_buff == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed!");
        
    }

    int index = 0;
    int timeout = 0;    //超过一定时间无声音则停止录音
    int total_rec = 0; //录音时间

    while(1)
    {
        //读取管道的音频缓存到buffer 960k
        raw_stream_read(raw_read, (char *)buffer, audio_wn_chunksize * sizeof(short));    //960
        if(enable_wn)
        {
            //将音频数据输入模型
            if(wakenet->detect(model_wn_data, (int16_t *)buffer) ==  1)
            {
                ESP_LOGI(TAG, "wake up start listening");
                //匹配，唤醒
                enable_wn = false;
            }
        }
        else{

            //唤醒后，raw_stream_read继续读取音频到buffer
            if (recoder != NULL)
            {
                //判断 达到录音最长或停止说话
                if(total_rec<(MAX_RECODER-960)&&timeout<RECODER_TIMEOUT)
                {
                    //继续录音
                    //将buffer的音频复制到recoder
                    memcpy(recoder+(index*audio_wn_chunksize * sizeof(short)), buffer, audio_wn_chunksize * sizeof(short));
                    index++;
                    //记录总音频数据大小
                    total_rec += audio_wn_chunksize * sizeof(short);//max=131072
                    
                }
                else
                {
                    //停止录音 准备将音频发送到百度api
                    
                    ESP_LOGI(TAG, "stop listening");
                    memset(http_buff, 0, MAX_HTTP_LEN); //重置http buff
                    
                    //配置http_client
                    esp_http_client_config_t config={
                            .method=HTTP_METHOD_POST,   //post方式
                            .event_handler=http_event_handle,   //注册http回调函数
                            .user_data = (void *)http_buff, //传递参数
                    };
                    memset(url, 0, 200);
                    sprintf(url, BAIDU_ASR_URL, baidu_access_token);    //将token组装到url

                    config.url = url;
                    printf("start connect to url = %s\r\n",config.url);
                    //http连接开始准备
                    esp_http_client_handle_t client = esp_http_client_init(&config);
                    esp_http_client_set_header(client, "Content-Type", "audio/pcm;rate=16000"); //设置http头部
                    esp_http_client_set_post_field(client,(const char*)recoder,total_rec);  //将录音添加到http body
                    
                    ESP_LOGI(TAG,"start trasnlate");
                    esp_http_client_perform(client);    //执行http连接
                    esp_http_client_close(client);  //关闭清除 等待回调函数
                    esp_http_client_cleanup(client);

                    free(recoder);  //释放录音内存
                    recoder = NULL;
                    index = 0;
                    total_rec = 0;
                    timeout = 0;
                    enable_wn = true;   //进入睡眠，等下次唤醒

                    ESP_LOGI(TAG,"start Etymology_Analysis");
                    Etymology_Analysis();   //词法分析

                    Audio_Order order = build_order(parse_items()); //解析语音命令
                    //执行动作
                    switch(order.object)
                    {
                        case obj_Ac:
                            ac_order(order.option, order.number);
                            break;
                        case obj_Bt:
                            bt_order(order.option);
                            break;
                        case obj_Weather:

                            break;
                        default:
                            break;
                    }
                    
                }

            }
            else
            {
                recoder = malloc(MAX_RECODER);  //为录音分配内存
            }

            //复制buffer的音频数据到vad_buff
            memcpy(vad_buff, buffer, VAD_BUFFER_LENGTH * sizeof(short));
            
            //将vad_buff的音频输入到声音检测模型 
            vad_state_t vad_state = vad_process(vad_inst, vad_buff);

            //判断是否有声音
            if (vad_state == VAD_SPEECH) {
                //讲话未结束
                timeout = 0;
            }
            else
            {
                //计时
                timeout++;
            }
            //enable_wn=true;

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
    wakenet->destroy(model_wn_data);    //销毁模型
    model_wn_data = NULL;
    free(buffer);   //释放内存
    buffer = NULL;   

}

void ASR_Init(void)
{
    xTaskCreate(ASR_Task, "ASR_Task", 4096, NULL, 5, NULL); 
}