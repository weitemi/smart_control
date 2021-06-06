### 华为云iotDA

### 应用侧

##### 鉴权认证



project_id :0c9433636e000fe02f6fc00a6e3661f6

product_id:60b9c9383744a602a5cb9bf3

IAM用户名：hw12224441

IAM密码：5812870huawei

IAMDoaminId:hw12224441

region:cn-north-4



##### 下发设备命令

文档 https://support.huaweicloud.com/api-iothub/iot_06_v5_0038.html

命令1：空调控制

```c
{
  "service_id" : "ac_control",
  "command_name" : "ac_control",
  "paras" : {
      "ac_power":1,
      "ac_mode":0,
      "ac_wind_speed":0,
      "ac_temp":26

  }
}
```

![下发空调命令](picture/%E4%B8%8B%E5%8F%91%E7%A9%BA%E8%B0%83%E5%91%BD%E4%BB%A4.png)





##### 查询设备属性

![查询设备属性](picture/%E6%9F%A5%E8%AF%A2%E8%AE%BE%E5%A4%87%E5%B1%9E%E6%80%A7.png)