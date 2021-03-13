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

extern TaskHandle_t ASR_task_handle;

void ASR_Task(void *agr);

#endif