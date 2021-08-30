<!--
 * @Author: your name
 * @Date: 2021-03-06 12:29:43
 * @LastEditTime: 2021-08-30 23:32:11
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \myapp\off_asr\tools\readme.md
-->
## 工具

### mp3

用于生成本地mp3二进制文件,博客：https://blog.csdn.net/weixin_44821644/article/details/107934841

在mp3目录下，放需要的mp3文件，所有文件大小不要超过100k，然后执行

```bash
python2 mk_audio_bin.py
```

会在mp3目录下生成audio-esp.bin，main/audio目录下生成audio_tone_uri.c、audio_tone_uri.h，代码中调用该头文件即可使用mp3.

将audio-esp.bin依照partitions.csv中的地址烧录到芯片flash

### spiffs
生成spiffs文件系统，参考博客：https://blog.csdn.net/weixin_44821644/article/details/109480902

在spiffs目录下，binfile目录下存放着红外码库的二进制文件

在spiffs目录执行以下命令，生成文件系统spiffs.bin，文件系统大小为0x19000，100k

```bash
python spiffsgen.py 0x19000 binfile spiffs.bin

```
将spiffs.bin依照partitions.csv中的地址烧录到芯片flash