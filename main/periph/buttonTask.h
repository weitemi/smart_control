/*
 * @Author: your name
 * @Date: 2020-10-25 15:01:11
 * @LastEditTime: 2021-03-24 08:26:53
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\ASR\main\periph\buttonTask.h
 */
#ifndef _BUTTONTASK_
#define _BUTTONTASK_

#include "board.h"

#define LED_ON gpio_set_level(LED, 0) //led亮
#define LED_OFF gpio_set_level(LED, 1)  //LED灭

void button_task(void *agr);
int led_init();

#endif