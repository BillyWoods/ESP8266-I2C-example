/* Defined in i2c_master.h, SDA is GPIO2, and SCL is GPIO14
 *
 * On the nodeMCU board GPIO2 maps to D4, and GPIO14 maps to D5
 */

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "driver/i2c_master.h"

#define procTaskQueueLen 1
#define userProcTaskPrio 0
os_event_t procTaskQueue[procTaskQueueLen];
void loop(os_event_t* events);

void initHMC5883L();
void getRawData();

void ICACHE_FLASH_ATTR user_init()
{
    uart_div_modify(0, UART_CLK_FREQ/115200);
    
    i2c_master_gpio_init();
    i2c_master_init();
    os_delay_us(100000);
    initHMC5883L();

    system_os_task(loop, userProcTaskPrio, procTaskQueue, procTaskQueueLen);
    system_os_post(userProcTaskPrio, 0, 0);
    os_printf("Init done\r\n");
}

void loop(os_event_t* events)
{
    getRawData();
    os_delay_us(500000);
    system_os_post(userProcTaskPrio, 0, 0);
}

void beginTransmission(uint8_t i2c_addr)
{
    uint8_t i2c_addr_write = (i2c_addr << 1);
    i2c_master_start();
    i2c_master_writeByte(i2c_addr_write);
    i2c_master_setAck(1);
}

void endTransmission()
{
    i2c_master_stop();
}

void write(uint8_t i2c_data)
{
    i2c_master_writeByte(i2c_data);
    i2c_master_setAck(1);
}

void requestFrom(uint8_t i2c_addr, uint8_t num_bytes)
{
    uint8_t i2c_addr_read = (i2c_addr << 1) + 1;
    i2c_master_start();
    i2c_master_writeByte(i2c_addr_read);
    i2c_master_setAck(1);
    os_delay_us(100);
    //i2c_master_writeByte(6);//num_bytes);
    //i2c_master_setAck(1);
    //i2c_master_stop();
}

const uint8_t HMC5883LRootAddress = 0x1E;

void initHMC5883L()
{   
    //set sensitivity range for +-1.3Ga
    beginTransmission(HMC5883LRootAddress);
    write(0x01);
    write(0x20);
    endTransmission();
    
    os_delay_us(100);
    
    //set register a for 8x oversampling
    beginTransmission(HMC5883LRootAddress);
    write(0x00);
    write(0x70);
    endTransmission();
    
    os_delay_us(100);
    
    //set continuous operation mode  
    beginTransmission(HMC5883LRootAddress);
    write(0x02);
    write(0x00);
    endTransmission();
    
    os_delay_us(100);
}

int16_t compassData[3] = {0,0,0};
void getRawData()
{
    const uint8_t i2cTimeout = 20;
    //uint8_t currentWaitTime = 0;

    //move register pointer to start of X reg
    beginTransmission(HMC5883LRootAddress);
    write(0x03);
    endTransmission();

    requestFrom(HMC5883LRootAddress, 6);
    os_delay_us(100);

    int i = 0;
    
    while (i < 3)
    {
	compassData[i] = i2c_master_readByte() << 8;
	i2c_master_send_ack();
        os_delay_us(100);
	compassData[i] |= i2c_master_readByte();
	i2c_master_send_ack();
	i++;
        os_delay_us(100);
    }

    endTransmission();

    os_printf( "X: %d\r\nY: %d\r\nZ: %d\r\n",
              compassData[0],
	      compassData[2],
	      compassData[1] );
    os_delay_us(68000);
}
