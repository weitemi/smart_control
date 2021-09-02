/*
 * @Author: your name
 * @Date: 2021-09-02 10:13:49
 * @LastEditTime: 2021-09-02 20:12:42
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \smart_control\main\app\etymology.h
 */
#ifndef _ETYMOLOGY_H
#define _ETYMOLOGY_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
  day:今天=1、明天=2、后天=3
  sig:早上=0、下午晚上=1
  hour:0-12
  min:0-60
  obj:操作对象宾语 空调=1、蓝牙=2、天气=3
  action:动作谓语 打开=1、关闭=0
  number:语句中的数字
*/
#define OBJ_AIR_CONDITION 1
#define OBJ_BLE 2
#define OBJ_WEATHER 3
#define ACTION_ON 1
#define ACTION_OFF 0
#define TIME_TODAY 1
#define TIME_TOMORROW 2
#define TIME_ADFTERMORROW 3
#define SIG_AM 0
#define SIG_PM 1

struct time
{
	unsigned short day : 2;
	unsigned short sig : 1;
	unsigned short hour : 4;
	unsigned short min : 9;
};
typedef struct time time_tt;

struct order
{
    time_tt time;
    unsigned short obj:3;
    unsigned short action:2;
	unsigned short number:11;
};
typedef struct order order_t;   //4字节



order_t etymology(char *s);

#endif