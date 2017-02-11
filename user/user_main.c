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
