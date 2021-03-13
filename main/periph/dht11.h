#ifndef _DHT11_H
#define _DHT11_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "soc/rmt_reg.h"

int dht11_rmt_rx(int gpio_pin, int rmt_channel,
                 int *humidity, int *temp_x10);
int dht11_rmt_rx_init(int gpio_pin, int channel);

#endif
