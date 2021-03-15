<!--
 * @Author: your name
 * @Date: 2021-03-06 09:55:31
 * @LastEditTime: 2021-03-15 14:22:26
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\README.md
-->
# 智能控制

## 使用：
根据partitions.cv烧录二进制文件，其中包括app，spiffs文件系统，和flash音频文件。

### 1，烧录spiffs文件系统
项目目录下执行 `python spiffsgen.py 0x19000 binfile spiffs.bin` ，其中的0x19000是分区表设置的文件系统的大小，为100k。然后使用esp32烧录工具烧录到分区表所示的位置(0x330000)。

> python D:/ESP/idf3.3/components/esptool_py/esptool/esptool.py --chip esp32 --port COM4 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x330000 ./spiffs.bin

### 2,烧录mp3

将mp3文件放入tools目录下，安装python2，运行`python2 mk_audio_bin.py`，生成esp-audio.bin文件<br>

项目目录，执行例如`python D:\ESP\esp-idf-v4.0.2\components\esptool_py\esptool\esptool.py --chip esp32 --port COM4 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x320000 ./tools/audio-esp.bin`

## 功能简介

### 语音唤醒及识别
嗨乐鑫，唤醒，蓝灯亮起，执行相应动作：
> 空调控制
> 互联网数据
> 定时任务设置

### 红外学习

通过按键进入红外接收，使用遥控器发射“打开 制冷模式 26° 自动扫风 一级风速”即可学习对应协议，目前仅支持格力美的海尔


## 目前的问题

wifi csi及蓝牙


