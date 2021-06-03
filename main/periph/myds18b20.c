#include "myds18b20.h"




/*
 * reset the ds18b20 
 */
static int ds18b20_reset()
{
    int err = 0;
    gpio_set_direction(DB18B20_PIN,GPIO_MODE_OUTPUT);
    gpio_set_level(DB18B20_PIN, 1);
    ets_delay_us(1000);
    gpio_set_level(DB18B20_PIN, 0);
    ets_delay_us( 600 );
    gpio_set_level(DB18B20_PIN, 1);

    gpio_set_direction(DB18B20_PIN,GPIO_MODE_INPUT);
    ets_delay_us(30);
    while(gpio_get_level(DB18B20_PIN)&&err<100)
    {
        ets_delay_us( 1 );
        err++;
    }
    ets_delay_us(470);
    while(!gpio_get_level(DB18B20_PIN))
    {
        ets_delay_us( 1 );

    }
    
    return 1;
}

/*
 * write 1 byte to ds18b20
 */ 
static int ds18b20_write_byte(uint8_t data)
{
    int i = 0;
    uint8_t temp = 0;
    gpio_set_direction(DB18B20_PIN,GPIO_MODE_OUTPUT);
    for (i=0; i < 8;i++)
    {
        gpio_set_level(DB18B20_PIN, 0);
        ets_delay_us(5);
        //temp = data & 0x01;
        gpio_set_level(DB18B20_PIN, data & 0x01);  //写数据并保持至少60us
        ets_delay_us(80);
        gpio_set_level(DB18B20_PIN, 1); //释放总线
        data >>= 1;
    }
    ets_delay_us(100);
    return 1;
}


/*
 * read 1 byte to ds18b20
 */ 
static unsigned char ds18b20_read_byte()
{
    unsigned char temp = 0,bi = 0;
    int i = 0;

    for (i=0; i < 8; i++)
    {
        gpio_set_direction(DB18B20_PIN,GPIO_MODE_OUTPUT);
        gpio_set_level(DB18B20_PIN, 0);
        ets_delay_us(2);
        gpio_set_level(DB18B20_PIN, 1);
        ets_delay_us(15);
        gpio_set_direction(DB18B20_PIN,GPIO_MODE_INPUT);    //释放总线
        
        if(gpio_get_level(DB18B20_PIN)) temp|=0x01<<i;
        
        ets_delay_us(15);
    }
    return temp;
}


/*
 * get temperature data from ds18b20
 * note:task will be suspend during geting data
 */ 
float ds18b20_get_data()
{
    char tl = 0, th = 0;
    float temp = 0;
    if(ds18b20_reset()!=1)
        return 0.0;
    //ets_delay_us(1000);
    ds18b20_write_byte(0xcc);   //跳过rom
    ds18b20_write_byte(0x44);   //温度转换指令
    vTaskDelay(100 / portTICK_RATE_MS); //!wait the change 等待温度转换完成

    if(ds18b20_reset()!=1)
        return 0.0;
    //ets_delay_us(1000);
    ds18b20_write_byte(0xcc);
    ds18b20_write_byte(0xbe);   //read temp command
    tl = ds18b20_read_byte();
    th = ds18b20_read_byte();

    temp=(float)(tl+(th*256))*0.0625;

    //printf("tl = 0x%x th= 0x%x", tl, th);

    return temp;

}




