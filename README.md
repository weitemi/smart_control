<!--
 * @Author: your name
 * @Date: 2021-03-06 09:55:31
 * @LastEditTime: 2021-03-25 10:18:23
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\README.md
-->


# 智能控制

## 一 使用：

### 编译

项目根目录下，执行 idf.py build

### 烧录

根据partitions.cv的地址安排，烧录二进制文件，可使用esptool.py或烧录软件，如图：

### 运行

#### 语音唤醒及识别
嗨乐鑫，唤醒，蓝灯亮起，说出命令词
> 空调控制：打开空调，设置空调27度，关闭空调

> 互联网数据：今天/明天/后天 天气怎么样

> 定时任务设置：5秒后打开空调 ，十秒后关闭空调

> 其他：现在几点，室内温度，红外学习

###### 命令词表

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


#### 红外学习

通过按键或语音命令进入红外接收，使用遥控器发射“打开 制冷模式 26° 自动扫风 一级风速”即可学习对应协议，目前仅支持格力美的海尔

#### 蓝牙配网

打开微信，搜索小程序BLETool,打开蓝牙，连接到ESP32A1S，然后在characteristics分别写入：

> 6601 :写入你的路由名称
> 6602:路由密码
> 6603:发送666确认连接

完成以上操作后。设备会自动连接到路由器，复位后会尝试连接上次连接的路由器

## 二 目录介绍

### 1,binfile

存放红外码库的二进制文件，在项目根目录下有`spiffsgen.py`，用于把binfile文件编辑成可烧录到esp32的spiffs.bin，将该二进制文件烧录到esp32，则可使用spiffs文件系统

项目目录下执行 `python spiffsgen.py 0x19000 binfile spiffs.bin` ，其中的0x19000是分区表设置的文件系统的大小，为100k。


### 2，tools

用于生成二进制音频文件；

将mp3文件放入tools目录下，安装python2，运行`python2 mk_audio_bin.py`，生成esp-audio.bin文件





## 目前的问题

wifi csi及蓝牙 语音识别精度不够







