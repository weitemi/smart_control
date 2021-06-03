/*
 * @Author: prx
 * @Date: 2020-12-19 16:48:34
 * @LastEditTime: 2021-06-04 00:03:10
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adfd:\MyNote\clk_t\clk_t.h
 */
#ifndef _CLOCK_H
#define _CLOCK_H

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "mywifi.h"

#define CLK_TASK_SIZE 2048
#define CLK_TASK_PRO 5

//日历结构体
struct calendar
{
	unsigned int second : 6 ;	//低位
	unsigned int minute : 6 ;
	unsigned int hour : 5 ;
	unsigned int date : 5 ;
	unsigned int month : 4 ;
	unsigned int year : 6 ;	//高位
};


//时间结构体，cal的值决定了value
union clk
{
	struct calendar cal;
	unsigned int value;
};

typedef union clk clk_t;
struct timer;
typedef void (*timer_cb)(struct timer *tmr ,void *arg);

//定时任务结构体
struct timer
{
	char *name;	//名称
    struct timer *next;	//指向下一个定时
    clk_t timeout;	//定时的时间
    timer_cb cb;	//回调函数
    void *arg;	//回调参数
};


void printf_tmrlist();	//打印定时列表
struct timer *tmr_new(clk_t *conf,timer_cb cb,void *arg,char *name); //新建闹钟

int tmr_add(struct timer *tmr);	//添加定时任务
int tmr_remove(struct timer *tmr);	//移除定时任务
int tmr_delete(struct timer *tmr);	//删除定时任务

void tmr_process(void *arg);	//定时任务处理


//
void clk_init();
void update_clk();
uint32_t get_clk();	//获取时间

#endif
