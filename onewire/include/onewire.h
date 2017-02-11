#ifndef __ONEWIRE_H__
#define __ONEWIRE_H__

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"

// The SDK name of the ESP8266 pin to use.
#define ONEWIRE_MUX     PERIPHS_IO_MUX_MTMS_U
// The function to assign to the pin (must be a GPIO function).
#define ONEWIRE_FUNC    FUNC_GPIO14
// The SDK pin number of the ESP8266 GPIO PIN we're using, must match above.
#define ONEWIRE_PIN     14

#define ONEWIRE_SEARCH_ROM      0xF0
#define ONEWIRE_READ_ROM        0x33
#define ONEWIRE_MATCH_ROM       0x55
#define ONEWIRE_SKIP_ROM        0xCC
#define ONEWIRE_ALARMSEARCH     0xEC

#define ROM_BYTES 8
#define BITS_PER_BYTE 8

#define ONEWIRE_SEARCH_FOUND        0x00
#define ONEWIRE_SEARCH_NO_DEVICES   0x01
#define ONEWIRE_SEARCH_CRC_INVALID  0x02


// State for a 1wire bus search.
struct onewire_search_state {
    uint8 address[8];
    int32 lastDiscrepancy;
    uint32 lastDeviceFlag;
};

void onewire_init();
void onewire_init_search_state(struct onewire_search_state *state);

uint32 onewire_reset();

void onewire_write(uint8 v, uint32 power);
void onewire_write_bit(uint32 v);
uint8 onewire_read();
uint32 onewire_read_bit();

uint32 onewire_search(struct onewire_search_state *state);
void onewire_select(const uint8 rom[8]);
void onewire_skip();

uint8 crc8(const uint8 *addr, uint8 len);

#endif
