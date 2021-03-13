/*
 * @Author: prx
 * @Date: 2020-12-19 16:48:34
 * @LastEditTime: 2021-03-07 14:36:18
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
void clk_t_q();

struct calendar
{
	unsigned int second : 6 ;	//低位
	unsigned int minute : 6 ;
	unsigned int hour : 5 ;
	unsigned int date : 5 ;
	unsigned int month : 4 ;
	unsigned int year : 6 ;	//高位
};


//使用位域

union clk
{
	struct calendar cal;
	unsigned int value;
};
typedef union clk clk_t;
struct timer;
typedef unsigned char (*timer_cb)(struct timer *tmr ,void *arg);

struct timer
{
	char *name;
    struct timer *next;
    clk_t timeout;
    timer_cb cb;
    void *arg;
};



extern clk_t global_clk;
extern struct timer *timer_list;
void printf_tmrlist();
struct timer *tmr_new(clk_t *conf,timer_cb cb,void *arg,char *name); //新建闹钟
int update_clk(clk_t *clk);
int tmr_add(struct timer *tmr);
int tmr_remove(struct timer *tmr);
int tmr_delete(struct timer *tmr);
void tmr_set_global(clk_t conf);
clk_t get_current_nettime();
int global_clk_init();
void tmr_process(void *arg);

void clock_task(void *arg);
#endif
