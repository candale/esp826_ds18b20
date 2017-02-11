# ESP8266 NO-SDK DS18B20 Library

This library is here because I could not find a standalone library for this sensor; all that I found was embedded in other projects making them unusable with git submodule.

The onewire library present in this repository is taken from [n0bel's github repository](https://github.com/n0bel/esp8266-UdpTemp). It will soon go into another, separate repository so it may be used in other projects as well. 
It will also suffer some changes to support communication on any pin (rather than the hardcoded 14 pin) and maybe better API.

The DS18B20 library was written by me having as guideline the DallasTemperature library and other useful resources linked below (if you want to learn how the sensor works).


### API:

```c
/**
Init a DS18B20_Sensors data structure
**/
void ds18b20_init(DS18B20_Sensors* sensors);

/**
Discover all DS18B20 sensors on the wire and keep their addresses in passed structure.

Return the number of sensors found.
**/
int ds18b20_get_all(DS18B20_Sensors* sensors);

/**
Return temperature from sensor number `target`.
**/
float ds18b20_read(DS18B20_Sensors*, uint8_t target);

/**
Set resolution on sensor number `target` and persist it in EEPROM.
Choices for resolution:
    #define DS18B20_TEMP_9_BIT  0x1F //  9 bit
    #define DS18B20_TEMP_10_BIT 0x3F // 10 bit
    #define DS18B20_TEMP_11_BIT 0x5F // 11 bit
    #define DS18B20_TEMP_12_BIT 0x7F // 12 bit

Return if the operations was successful or not.
**/
uint8_t ds18b20_set_resolution(DS18B20_Sensors* sensors, uint8_t target, uint8_t resolution);

/**
Return the resolution of sensor number `target`
**/
uint8_t ds18b20_get_resolution(DS18B20_Sensors* sensors, int target);
```


### Usage

```c
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "wifi.h"
#include "mem.h"
#include "user_interface.h"
#include "onewire.h"
#include "ds18b20.h"

LOCAL os_timer_t read_timer;
DS18B20_Sensors sensors;

void ICACHE_FLASH_ATTR
print_temp(void* arg) {
    float tmp = ds18b20_read(&sensors, 0);
    INFO("Temp %d\n", (int)(tmp * 10000));
}

void ICACHE_FLASH_ATTR
init_all(void) {
    // system_set_os_print(0);
    uart_div_modify(0, UART_CLK_FREQ / 115200);

    ds18b20_init(&sensors);
    INFO("Found %d sensors\n", ds18b20_get_all(&sensors));
    INFO("Changing resolution to 12 bit\n");
    ds18b20_set_resolution(&sensors, 0, DS18B20_TEMP_12_BIT);

    os_timer_disarm(&read_timer);
    os_timer_setfn(&read_timer, (os_timer_func_t *)print_temp, 0);
    os_timer_arm(&read_timer, 1500, 1);

}

void ICACHE_FLASH_ATTR
user_init()
{
    system_init_done_cb(init_all);
}
```


Limitations:

- Currently reads only on pin 14 and it is not configurable.

ToDO:

- Separate onewire library into another repository.


#### References (for didactic purposes):

[1] http://www.homautomation.org/2015/11/17/ds18b20-how-to-change-resolution-9101112-bits/

[2] https://tushev.org/articles/arduino/10/how-it-works-ds18b20-and-arduino

[3] https://github.com/milesburton/Arduino-Temperature-Control-Library

[4] http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
