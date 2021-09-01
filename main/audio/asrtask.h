/*
 * @Author: your name
 * @Date: 2021-03-24 23:46:44
 * @LastEditTime: 2021-09-01 20:41:02
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \asr\main\audio\asrtask.h
 */
#ifndef _ASRTASK_H
#define _ASRTASK_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdio.h"
#include "audio_pipeline.h"
#include "esp_system.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_log.h"



#include "i2s_stream.h"
#include "raw_stream.h"
#include "tone_stream.h"
#include "audio_tone_uri.h"
#include "mp3_decoder.h"
#include "filter_resample.h"
#include "rec_eng_helper.h"
#include "esp_vad.h"

#include "player.h"

void ASR_Init(void);

#endif