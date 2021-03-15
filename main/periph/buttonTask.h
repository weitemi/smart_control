/*
 * @Author: your name
 * @Date: 2020-10-25 15:01:11
 * @LastEditTime: 2021-03-15 10:09:36
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\ASR\main\periph\buttonTask.h
 */
#ifndef _BUTTONTASK_
#define _BUTTONTASK_
#include "esp_log.h"
#include "board.h"
#include "esp_peripherals.h"
#include "periph_touch.h"
#include "periph_adc_button.h"
#include "periph_button.h"
#include "irTask.h"
#include "mynvs.h"
#include "esp_audio.h"





extern TaskHandle_t buttonTask_handle;
void button_task(void *agr);


#endif