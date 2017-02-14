#ifndef __DS18B20_H__
#define __DS18B20_H__

/**
URL for explaining how the reading from this device works
https://tushev.org/articles/arduino/10/how-it-works-ds18b20-and-arduino
**/

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "onewire.h"
#include "mem.h"

#define DS18B20_CONVERT_T           0x44
#define DS18B20_WRITE_SCRATCHPAD    0x4E
#define DS18B20_READ_SCRATCHPAD     0xBE
#define DS18B20_COPY_SCRATCHPAD     0x48
#define DS18B20_READ_EEPROM         0xB8
#define DS18B20_READ_PWRSUPPLY      0xB4

#define DS18B20_TEMP_9_BIT  0x1F //  9 bit
#define DS18B20_TEMP_10_BIT 0x3F // 10 bit
#define DS18B20_TEMP_11_BIT 0x5F // 11 bit
#define DS18B20_TEMP_12_BIT 0x7F // 12 bit

#define DS18B20_ROM_IDENTIFIER      0x28

#define ADDR_LEN 8
#define DS18B20_GROW_RATIO       2
#define DS18B20_INIT_ADDR_LENGTH 5
#define DS18B20_ADDR_SIZE ADDR_LEN * sizeof(uint8)

typedef struct _sensors {
    uint8_t* addresses;
    uint8_t count;
    size_t length;
    // not currently used
    uint8_t parasite_mode;
} DS18B20_Sensors;

void ICACHE_FLASH_ATTR
ds18b20_init(DS18B20_Sensors* sensors);

int ICACHE_FLASH_ATTR
ds18b20_get_all(DS18B20_Sensors* sensors);

void ICACHE_FLASH_ATTR
ds18b20_request_temperatures(DS18B20_Sensors* sensors);

float ICACHE_FLASH_ATTR
ds18b20_read(DS18B20_Sensors*, uint8_t target);

uint8_t ICACHE_FLASH_ATTR
ds18b20_set_resolution(DS18B20_Sensors* sensors, uint8_t target, uint8_t resolution);

uint8_t ICACHE_FLASH_ATTR
ds18b20_get_resolution(DS18B20_Sensors* sensors, int target);


#endif
