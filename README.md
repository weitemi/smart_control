<!--
 * @Author: your name
 * @Date: 2021-03-06 09:55:31
 * @LastEditTime: 2021-06-06 15:44:00
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\README.md
-->


# 智能控制

## 一 使用：

### 编译

本项目基于idf-v4.0.2，使用adf框架

项目根目录下，执行 idf.py build

### 烧录

根据partitions.cv的地址安排，烧录二进制文件，可使用esptool.py或烧录软件，如图：

http://m.qpic.cn/psc?/V52o58sF1Om7WA2cK9TD1YG4la1SnXbV/45NBuzDIW489QBoVep5mcf7e8kPaoZhI6FN*E3CXjG0OsGC4XS6GolTsqSDGzWFzS9HX8V8a2VPETFG.PJQiCPOIAwhut2faMHhCHYXzL1I!/b&bo=cQfJAAAAAAADF40!&rf=viewer_4

### 运行

上电后会自动连接上次连接的路由器 若无法连接，则会一直请求连接

#### 蓝牙配网

打开微信，搜索小程序BLETool,打开蓝牙，连接到ESP32A1S，然后在characteristics分别写入：

> 6601 :写入你的路由名称
> 6602:路由密码
> 6603:发送666确认连接

完成以上操作后。设备会自动连接到路由器，复位后会尝试连接上次连接的路由器。配网完成后可关闭BLE以节省内存资源

开发者也可通过串口发送如下串口数据实现配网

> #yourssid
> @yourpassword
> $

#### 语音唤醒及识别

嗨乐鑫，唤醒，蓝灯亮起，说出命令词

命令词及控制代码在 'static esp_err_t asr_multinet_control(int commit_id)'

#### 红外遥控/学习

开启`ir_study();`后，使用本地红外遥控器发送指定命令（如，空调遥控器发送`打开 制冷模式 26° 自动扫风 一级风速`，目前仅支持格力美的海尔）即可匹配本地码库。

目前支持空调红外遥控，如设置开关，温度，扫风，风速，工作模式。

#### 日历系统

主要在clock.c中，以一个任务的形式存在。日历系统在初始化阶段会获取网络gmt时间，并设置一个定时器，每隔24小时获取gmt时间。

日历系统支持定时器功能，定时时间精度为1s，定时最大时长可1年（不建议）

#### http请求

该部分代码位于myhttp.c中，主要负责网络时间，天气数据，百度token的获取。

#### MQTT客户端

使用华为云iot接入

文档 https://support.huaweicloud.com/api-iothub/iot_06_v5_3010.html
##### 主题列表

**下发设备命令：**
$oc/devices/60b9c9383744a602a5cb9bf3_smart_control_01/sys/commands/request_id={request_id}

设备响应：
$oc/devices/60b9c9383744a602a5cb9bf3_smart_control_01/sys/commands/response/request_id={request_id}

**查询设备属性：**
$oc/devices/{device_id}/sys/properties/get/request_id={request_id}

设备响应
$oc/devices/{device_id}/sys/properties/get/response/request_id={request_id}

##### 命令控制


可汇报数据：室内温度，空调状态，温度，定时任务？
 - temp:(int)36
 - ir_status:(int)open,temperature,windspeed,
 - timer: (string)timer_name,(string)time

可控制的动作：空调的控制，读取温度，设置定时任务，红外学习
 - ir_status:(int)open,temperature,windspeed,
 - timer: (string)timer_name,(string)time
 - ir_study:(int) open

#### 传感器

目前搭载了ds18b20温度传感器，在每次上电时先读一次温度。



#### 命令词表

CONFIG_CN_SPEECH_COMMAND_ID0="she zhi kong tiao er shi du"
CONFIG_CN_SPEECH_COMMAND_ID1="she zhi kong tiao er shi yi du"
CONFIG_CN_SPEECH_COMMAND_ID2="she zhi kong tiao er shi er du"
CONFIG_CN_SPEECH_COMMAND_ID3="she zhi kong tiao er shi san du"
CONFIG_CN_SPEECH_COMMAND_ID4="she zhi kong tiao er shi si du"
CONFIG_CN_SPEECH_COMMAND_ID5="she zhi kong tiao er shi wu du"
CONFIG_CN_SPEECH_COMMAND_ID6="she zhi kong tiao er shi liu du"
CONFIG_CN_SPEECH_COMMAND_ID7="she zhi kong tiao er shi qi du"
CONFIG_CN_SPEECH_COMMAND_ID8="she zhi kong tiao er shi ba du"
CONFIG_CN_SPEECH_COMMAND_ID9="qi dong kong tiao sao feng"
CONFIG_CN_SPEECH_COMMAND_ID10="ting zhi kong tiao sao feng"
CONFIG_CN_SPEECH_COMMAND_ID11="she zhi kong tiao zi dong feng su"
CONFIG_CN_SPEECH_COMMAND_ID12="she zhi kong tiao yi ji feng su"
CONFIG_CN_SPEECH_COMMAND_ID13="she zhi kong tiao er ji feng su"
CONFIG_CN_SPEECH_COMMAND_ID14="she zhi kong tiao san ji feng su"
CONFIG_CN_SPEECH_COMMAND_ID15="yi xiao shi hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID16="liang xiao shi hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID17="san xiao shi hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID18="si xiao shi hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID19="wu xiao shi hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID20="liu xiao shi hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID21="qi xiao shi hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID22="ba xiao shi hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID23="jiu xiao shi hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID24="shi xiao shi hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID25="da kai kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID26="guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID27=""
CONFIG_CN_SPEECH_COMMAND_ID28=""
CONFIG_CN_SPEECH_COMMAND_ID29=""
CONFIG_CN_SPEECH_COMMAND_ID30="da kai lan ya"
CONFIG_CN_SPEECH_COMMAND_ID31="guan bi lan ya"
CONFIG_CN_SPEECH_COMMAND_ID32="ming tian tian qi zen me yang"
CONFIG_CN_SPEECH_COMMAND_ID33="jin tian tian qi zen me yang"
CONFIG_CN_SPEECH_COMMAND_ID34="hou tian tian qi zen me yang"
CONFIG_CN_SPEECH_COMMAND_ID35="shi nei wen du"
CONFIG_CN_SPEECH_COMMAND_ID36="xian zai ji dian"
CONFIG_CN_SPEECH_COMMAND_ID37="hong wai xue xi"
CONFIG_CN_SPEECH_COMMAND_ID38=""
CONFIG_CN_SPEECH_COMMAND_ID39=""
CONFIG_CN_SPEECH_COMMAND_ID40="shi miao hou guan bi kong tiao"
CONFIG_CN_SPEECH_COMMAND_ID41="jiu miao hou da kai kong tiao"




## 二 目录介绍

### 1,binfile

存放红外码库的二进制文件，在项目根目录下有`spiffsgen.py`，用于把binfile文件编辑成可烧录到esp32的spiffs.bin，将该二进制文件烧录到esp32，则可使用spiffs文件系统

项目目录下执行 `python spiffsgen.py 0x19000 binfile spiffs.bin` ，其中的0x19000是分区表设置的文件系统的大小，为100k。


### 2，tools

用于生成二进制音频文件；

将mp3文件放入tools目录下，安装python2，运行`python2 mk_audio_bin.py`，生成esp-audio.bin文件





## 目前的问题

语音识别精度不够









