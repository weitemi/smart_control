/*
 * @Author: your name
 * @Date: 2020-10-25 15:01:11
 * @LastEditTime: 2021-06-03 19:50:59
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\ASR\main\periph\buttonTask.h
 */
#ifndef _GERNERAL_GPIO_H_
#define _GERNERAL_GPIO_H_

#include "esp_peripherals.h"
#include "periph_button.h"
#include "esp_log.h"
#include "board.h"
#include "driver/gpio.h"

#define LED_ON gpio_set_level(LED, 0) //led亮
#define LED_OFF gpio_set_level(LED, 1)  //LED灭

#define BUTTON_TASK_PRO 4
#define BUTTON_TASK_SIZE 4096

void button_task(void *agr);
int General_Gpio_init(esp_periph_set_handle_t set);

#endif