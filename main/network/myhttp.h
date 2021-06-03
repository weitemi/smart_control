/*
 * @Author: your name
 * @Date: 2020-10-22 10:00:03
 * @LastEditTime: 2021-06-03 20:19:24
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\ASR\main\network\myhttp.h
 */
#ifndef _MYHTTP_H
#define _MYHTTP_H

#include "esp_http_client.h"
#include "http_stream.h"
#include "cJSON.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"



extern char baidu_access_token[80];
extern esp_err_t http_event_handle(esp_http_client_event_t *evt);
char *get_Weather_String(int day);
char *get_Time_String();
void http_api_task(void *arg);
void http_asr(char *recoder);
void http_init();
int update_access_token();
#endif
