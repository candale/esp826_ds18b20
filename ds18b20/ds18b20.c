#include "ds18b20.h"


struct onewire_search_state onewire_state;

uint8_t ICACHE_FLASH_ATTR
read_scratchpad(uint8_t* address, uint8_t* data) {
    uint8_t i;
    onewire_reset();
    onewire_select(address);
    onewire_write(DS18B20_READ_SCRATCHPAD, 0);

    for(i = 0; i < 9; i++) {
        data[i] = onewire_read();
    }

    return onewire_reset() == 1;
}


uint8_t ICACHE_FLASH_ATTR
is_connected(uint8_t* address, uint8_t* data) {
    uint8_t result = read_scratchpad(address, data);
    return result && crc8(data, 8) == data[8];
}


void ICACHE_FLASH_ATTR
ds18b20_init(DS18B20_Sensors* sensors) {
    sensors->addresses = (uint8_t*)os_zalloc(DS18B20_ADDR_SIZE * DS18B20_INIT_ADDR_LENGTH);
    sensors->count = 0;
    sensors->length = DS18B20_INIT_ADDR_LENGTH;
    sensors->parasite_mode = 0;
}


int ICACHE_FLASH_ATTR
ds18b20_get_all(DS18B20_Sensors* sensors) {
    onewire_init_search_state(&onewire_state);
    onewire_init();

    while(!onewire_state.lastDeviceFlag) {
        uint8_t search_status = onewire_search(&onewire_state);
        if(search_status == ONEWIRE_SEARCH_NO_DEVICES) {
            return 0;
        }

        if(search_status != ONEWIRE_SEARCH_FOUND) {
            return -1;
        }

        // If it is not a DS18B20
        if(onewire_state.address[0] != DS18B20_ROM_IDENTIFIER) {
            continue;
        }

        os_memcpy(
            sensors->addresses + (sensors->count) * DS18B20_ADDR_SIZE,
            onewire_state.address,
            DS18B20_ADDR_SIZE
        );

        if(sensors->count + 1 >= sensors->length) {
            uint8_t new_length;
            if(sensors->length * DS18B20_GROW_RATIO > 0xFF) {
                new_length = 0xFF;
            } else {
                new_length = sensors->length * DS18B20_GROW_RATIO;
            }

            sensors->addresses = (uint8_t*)os_realloc(
                sensors->addresses,
                sensors->length * DS18B20_ADDR_SIZE + (new_length * DS18B20_ADDR_SIZE)
            );

            sensors->length = new_length;
        }
        sensors->count++;
    }

    return sensors->count;
}


uint8_t ICACHE_FLASH_ATTR
ds18b20_set_resolution(DS18B20_Sensors* sensors, uint8_t target, uint8_t resolution) {
    uint8_t is_correct = (
        resolution == DS18B20_TEMP_9_BIT || resolution == DS18B20_TEMP_10_BIT ||
        resolution == DS18B20_TEMP_11_BIT || resolution == DS18B20_TEMP_12_BIT
    );

    if(!is_correct) {
        return 0;
    }

    if(ds18b20_get_resolution(sensors, target) == resolution) {
        return 1;
    }

    uint8_t* target_addr = sensors->addresses + target * DS18B20_ADDR_SIZE;

    onewire_reset();
    onewire_select(target_addr);

    // Write scratchpad
    onewire_write(DS18B20_WRITE_SCRATCHPAD, sensors->parasite_mode);
    onewire_write(0x00, sensors->parasite_mode); // user byte 1
    onewire_write(0x00, sensors->parasite_mode); // user byte 2
    onewire_init(resolution, sensors->parasite_mode);
    onewire_reset();

    // Persist resolution to EEPROM
    onewire_select(target_addr);
    onewire_write(DS18B20_COPY_SCRATCHPAD, sensors->parasite_mode);
    onewire_reset();

    return 1;
}


uint8_t ICACHE_FLASH_ATTR
ds18b20_get_resolution(DS18B20_Sensors* sensors, int target) {
    uint8_t data[12];

    uint8_t* target_addr = sensors->addresses + target * DS18B20_ADDR_SIZE;
    read_scratchpad(target_addr, data);

    return data[4];
}


float ICACHE_FLASH_ATTR
ds18b20_read(DS18B20_Sensors* sensors, uint8_t target) {
    if(target >= sensors->count) {
        return -1;
    }

    uint8_t* target_addr = sensors->addresses + target * DS18B20_ADDR_SIZE;
    uint8_t data[12];
    uint8_t i;

    if(!is_connected(target_addr, data)) {
        return -1;
    }

    // Tell sensor to prepare data
    onewire_reset();
    onewire_select(target_addr);
    onewire_write(DS18B20_CONVERT_T, sensors->parasite_mode);

    // Wait for it to process the data. May take up to 750 ms
    os_delay_us(850 * 1000);
    read_scratchpad(target_addr, data);

    uint32_t lsb = data[0];
    uint32_t msb = data[1];

    // Taken from Arduino DallasTemperature
    int16_t raw_temp = (((int16_t) msb) << 11) | (((int16_t) lsb) << 3);
    return raw_temp * 0.0078125;
}
