/*
 * @Author: your name
 * @Date: 2021-09-01 20:52:35
 * @LastEditTime: 2021-09-02 11:29:48
 * @LastEditors: your name
 * @Description: In User Settings Edit
 * @FilePath: \smart_control\main\periph\myds18b20.h
 */
#ifndef _MYDS18B20_H
#define _MYDS18B20_H
#include "main.h"


#define DB18B20_PIN GPIO_NUM_19

float ds18b20_get_data();

#endif