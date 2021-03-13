#ifndef _MYDS18B20_H
#define _MYDS18B20_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define DB18B20_PIN GPIO_NUM_19

float ds18b20_get_data();

#endif