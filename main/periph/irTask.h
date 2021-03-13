#ifndef _IRTASK_
#define _IRTASK_
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"
#include "mynvs.h"
#include "ir_decode.h"
#include "esp_spiffs.h"

#include "driver/gpio.h"

#define RMT_RX_ACTIVE_LEVEL  0   /*!< If we connect with a IR receiver, the data is active low */
#define RMT_TX_CARRIER_EN    1   /*!< Enable carrier for IR transmitter test with IR led */

#define RMT_TX_CHANNEL    1     /*!< RMT channel for transmitter */
#define RMT_TX_GPIO_NUM  14     /*!< GPIO number for transmitter signal */
#define RMT_RX_CHANNEL    3     /*!< RMT channel for receiver */
#define RMT_RX_GPIO_NUM  4     /*!< GPIO number for receiver */
#define RMT_CLK_DIV      100    /*!< RMT counter clock divider  计数器每80M/100=1.25us计数1次*/
#define RMT_TICK_10_US    (80000000/RMT_CLK_DIV/100000)   /*!< RMT counter value for 10 us.(Source clock is APB clock) 可以定时10us*/

#define HEADER_HIGH_9000US    9000                         /*!< NEC protocol header: positive 9ms */
#define HEADER_LOW_4500US     4500                         /*!< NEC protocol header: negative 4.5ms*/
#define HEADER_HIGH_4300US    4300
#define HEADER_LOW_4300US    4300
#define HEADER_HIGH_7300US    7300
#define HEADER_LOW_5800US    5800
#define HEADER_HIGH_3000US   3000
#define HEADER_LOW_3000US   3000
#define HEADER_HIGH_4500US   4500



#define NEC_CONNECT_HIGH_US     646                         /*!< NEC protocol CONNECT: positive 0.6ms*/
#define NEC_CONNECT_LOW_US     20000                          /*!< NEC protocol CONNECT: negative 2ms*/
#define NEC_BIT_ONE_HIGH_US    646                         /*!< NEC protocol data bit 1: positive 0.56ms */
#define NEC_BIT_ONE_LOW_US    1643   /*!< NEC protocol data bit 1: negative 1.69ms */
#define NEC_BIT_ZERO_HIGH_US   646                         /*!< NEC protocol data bit 0: positive 0.56ms */
#define NEC_BIT_ZERO_LOW_US   516  /*!< NEC protocol data bit 0: negative 0.56ms */
#define NEC_BIT_END            646                         /*!< NEC protocol end: positive 0.56ms */
#define NEC_BIT_MARGIN         200                          /*!< NEC parse margin time */

#define NEC_ITEM_DURATION(d)  ((d & 0x7fff)*10/RMT_TICK_10_US)  /*!< Parse duration time from memory register value */
#define NEC_DATA_ITEM_NUM   70  /*!< NEC code item number: header + 35bit data + connect +32bit +end*/
#define RMT_TX_DATA_NUM  2    /*!< NEC tx test data number */
#define rmt_item32_tIMEOUT_US  21000   /*!< RMT receiver timeout value(us) 由于连接码的时间长度大约为20600us所以设置时间长点 */

#define SET_IR_DATA(data, wide, mask, setdata) \
    {                                          \
        data &= ~(wide << mask);               \
        data |= (setdata << mask);             \
    }

//红外数据编码掩码
#define IR0_MASK_OPEN 3
#define IR0_MASK_WIND_SPEED 4
#define IR0_MASK_WIND_SCAN 6
#define IR0_MASK_TEMP 8
#define IR0_MASK_TIMER 12

#define IR1_MASK_WIND_SCAN 0


//掩码的位数
#define IR0_WIDE_OPEN 0b1
#define IR0_WIDE_WIND_SPEED 0b11
#define IR0_WIDE_WIND_SCAN 0b1
#define IR0_WIDE_TEMP 0b1111
#define IR0_TIMER 0b11111111

#define IR1_WIDE_WIND_SCAN 0b1


//空调开关
#define IR_OPEN 1
#define IR_CLOSE 0


//风速调节
#define IR_AUTO_WIND 0
#define IR_LOW_WIND 1
#define IR_MID_WIND 2
#define IR_HIGH_WIND 3



typedef struct IR_Msg_t *IR_Msg_handle;

//红外信息的结构体
struct IR_Msg_t
{
    uint8_t temp;   //温度
    uint8_t open;   //开关
    uint8_t windspeed;  //风速
    uint8_t scan;   //扫风开关
    uint8_t timer_hour; //定时 小时
    uint8_t timer_min;  //定时 分钟
    uint32_t check; //校验码
    uint64_t data0; //第一帧红外码
    uint32_t data1; //第二帧红外码
    
};

//红外发射类型
typedef enum 
{
    IR_TX_TYPE_NVS = 1, //使用nvs库
    IR_TX_TYPE_MSG, //使用代码
    IR_TX_TYPE_IREXT,   //irext库
    IR_TX_TYPE_MAX
}ir_tx_type_t;
extern const char *ir_code_lib[];

enum ac_band
{
    band_gree = 0,
    band_meidi = 1,
    songxia = 2,
    band_dajin = 3,
    band_haier = 4,
    band_haixin = 5,
    band_aux = 6,
    band_max
};
enum ac_pro_code
{
    code_1 = 0,
    code_2 = 1,
    code_3 = 2,
    code_4 = 3,
    code_5 = 4,
   
    code_max
};
struct AC_Control
{
    uint8_t index;
    enum ac_band band;
    enum ac_pro_code pro_code;   //protol code
    t_remote_ac_status status;
};
struct AC_Control ac_handle;

struct RX_signal
{
    uint32_t item_num;
    uint32_t lowlevel;//us
    uint32_t highlevel_1;
    uint32_t highlevel_0;
    uint32_t encode;
};



//extern t_remote_ac_status ac_status;
//extern IR_Msg_handle ir_msg ;
extern TaskHandle_t ir_tx_handle,ir_rx_handle;

int ac_set_code_lib(enum ac_band b, enum ac_pro_code code);
int ac_set_temp(int temp);
int ac_open(bool open);
int ac_set_wind_speed(int speed);
int ac_set_swing(bool open);

void ir_study();    //开启学习
int IR_init();  //初始化
int storage_init(); //存储系统，文件系统初始化

//int ir_tx_msg();
//int ir_rx(const char *key);
//int ir_tx_nvs(const char *key);
//int ir_tx_irext(const char *fn);
void rmt_ir_txTask(void *agr);
void rmt_ir_rxTask(void *agr);
#endif