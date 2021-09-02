/*
 * @Author: your name
 * @Date: 2021-09-02 10:13:49
 * @LastEditTime: 2021-09-02 10:55:13
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \smart_control\main\app\etymology.h
 */
#ifndef _ETYMOLOGY_H
#define _ETYMOLOGY_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    unsigned short action:13;
};
typedef struct order order_t;   //4字节



order_t etymology(char *s);

#endif