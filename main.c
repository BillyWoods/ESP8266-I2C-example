/* Billy Woods 25/11/16
 *
 * i2c example for the ESP8266 using the open non-OS SDK; the 
 *  device being communicated with is a Honeywell HMC5883L
 *  magnetometer.
 *
 * ------------------------NOTES--------------------------------
 *
 * Defined in i2c_master.h: SDA is GPIO2, and SCL is GPIO14
 *   (these are not hardware limitations, however, as the ESP8266
 *   implements i2c in software, so any GPIO pins should work)
 *
 * On the nodeMCU board, GPIO2 maps to D4, and GPIO14 maps to D5
 *
 * 4.7k - 10k pullups should be put on SDA and SCL, otherwise the 
 *   ESP8266 tends to go haywire on boot and randomly during 
 *   operation. 
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

//--------------------------------------------------------------
// Wrapped the SDK's i2c functions so that they are similar to
//  (but not completely compatible with) Arduino's Wire library.
// 
// These are the predefinitions:
//--------------------------------------------------------------
bool beginTransmission(uint8_t i2c_addr);
bool requestFrom(uint8_t i2c_addr);
bool read(uint8_t num_bytes, int8_t* data);
bool write(uint8_t i2c_data);
void endTransmission();

//--------------------------------------------------------------
// functions for reading and controlling the HMC5883L and other
//   related stuff
//--------------------------------------------------------------
const uint8_t HMC5883LRootAddress = 0x1E;
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

bool beginTransmission(uint8_t i2c_addr)
{
    uint8_t i2c_addr_write = (i2c_addr << 1);
    
    i2c_master_start();
    i2c_master_writeByte(i2c_addr_write);
    
    return i2c_master_checkAck();
}

void endTransmission()
{
    i2c_master_stop();
}

bool write(uint8_t i2c_data)
{
    i2c_master_writeByte(i2c_data);
    
    return i2c_master_checkAck();
}

bool requestFrom(uint8_t i2c_addr)
{
    uint8_t i2c_addr_read = (i2c_addr << 1) + 1;
    
    i2c_master_start();
    i2c_master_writeByte(i2c_addr_read);
    
    return i2c_master_checkAck();
}

bool read(uint8_t num_bytes, int8_t* data)
{
    if (num_bytes < 1 || data == NULL) return false;

    int i;
    for(i = 0; i < num_bytes - 1; i++)
    {
        data[i] = i2c_master_readByte();
        i2c_master_send_ack();
    }
    // nack the final packet so that the slave releases SDA
    data[num_bytes - 1] = i2c_master_readByte();
    i2c_master_send_nack();

    return true;
}
    

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

void getRawData()
{
    int8_t packets[6] = {0,0,0,0,0,0};
    int16_t compassData[3] = {0,0,0};

    // move compass's internal register pointer to start of X reg before we read
    if ( beginTransmission(HMC5883LRootAddress) )
    {
        write(0x03);
        os_printf("Talking to HMC5883L...\r\n");
    }
    else
        os_printf("Error communicating with HMC5883L!\r\n");

    endTransmission();

    requestFrom(HMC5883LRootAddress);
    os_delay_us(10);
    read(6, packets);
    endTransmission();
    
    // assemble the 8 bit packets into 16 bit twos complement signed integers
    int i;
    for(i = 0; i < 3; i++)
    {
        // zeroing compass data at some point in time is crucial so that the 
        //   bitwise OR works as intended below
        compassData[i] =  packets[2*i] << 8;
        compassData[i] |= packets[2*i + 1];
    }

    // compass data in transmitted in order X, Z, Y
    os_printf("X: %d\r\nY: %d\r\nZ: %d\r\n",
              compassData[0],
              compassData[2],
              compassData[1] );
}
