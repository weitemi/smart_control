/*
 * @Author: your name
 * @Date: 2021-03-24 08:40:13
 * @LastEditTime: 2021-09-02 23:59:14
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \esp-adf\examples\myapp\off_asr\main\periph\myuart.c
 */
#include "myuart.h"
#include "driver/uart.h"
#include "mywifi.h"

const int RX_BUF_SIZE = 1024;
const char *TAG = "myuart";
/*
 * 串口接收任务 接收控制台数据
 */
void rx_task()
{
    //todo 能否用局部变量？
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    static char ssid[10]={0}, password[20]={0};
    
    while (1)
    {
        const int rxBytes = uart_read_bytes(UART_NUM_0, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0)
        {

            data[rxBytes] = 0;
            ESP_LOGI(TAG, "Read %d bytes: '%s'", rxBytes, data);
            //读取ssid
            if (data[0] == '#')
            {
       
                strncpy(ssid, (char *)&data[1], rxBytes - 1);
                ssid[rxBytes-1] = 0;
                ESP_LOGI(TAG, "get ssid : %s", ssid);
            }

            //读取密码
            else if (data[0] == '@')
            {

            
                strncpy(password, (char *)&data[1], rxBytes - 1);
                password[rxBytes-1] = 0;
                ESP_LOGI(TAG, "get password : %s", password);
            }

            //确认修改wifi
            else if (data[0] == '%')
            {
                wifi_update(ssid, password);

            }
     
        }
    }
    free(data);
}
/*
 * 串口接收初始化 用于接收控制台wifi
 *
 */
int uart_init()
{
    esp_err_t err;
    esp_log_level_set(TAG, ESP_LOG_INFO);
    
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,  
        .stop_bits = UART_STOP_BITS_1,  
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // We won't use a buffer for sending data.
    err = uart_driver_install(UART_NUM_0, RX_BUF_SIZE * 2, 0, 0, NULL, 0);

    xTaskCreate(rx_task, "uart_rx", 2048, NULL, 3, NULL);
    ESP_LOGI(TAG, "Uart Init OK");
    return err;
}

