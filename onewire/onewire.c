#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"

#include "onewire.h"

void ICACHE_FLASH_ATTR onewire_init() {
    // Set the configured pin as gpio pin.
    PIN_FUNC_SELECT(ONEWIRE_MUX, ONEWIRE_FUNC);
    // Enable pull-up on the configured pin.
    PIN_PULLUP_EN(ONEWIRE_MUX);
    // Set configured pin as an input
    GPIO_DIS_OUTPUT(ONEWIRE_PIN);
}

// Reset the search state
void ICACHE_FLASH_ATTR onewire_init_search_state(struct onewire_search_state *state) {
    state->lastDiscrepancy = -1;
    state->lastDeviceFlag = FALSE;
    os_memset(state->address, 0, sizeof(state->address));
}

// Perform the onewire reset function.  We will wait up to 250uS for
// the bus to come high, if it doesn't then it is broken or shorted
// and we return a 0;
// Returns 1 if a device asserted a presence pulse, 0 otherwise.
uint32 ICACHE_FLASH_ATTR onewire_reset() {
    uint32 result;
    uint8 retries = 125;

    // Disable output on the pin.
    GPIO_DIS_OUTPUT(ONEWIRE_PIN);

    // Wait for the bus to get high (which it should because of the pull-up resistor).
    do {
        if (--retries == 0) {
            return 0;
        }
        os_delay_us(2);
    } while (!GPIO_INPUT_GET(ONEWIRE_PIN));

    // Transmit the reset pulse by pulling the bus low for at least 480 us.
    GPIO_OUTPUT_SET(ONEWIRE_PIN, 0);
    os_delay_us(500);

    // Release the bus, and wait, then check for a presence pulse, which should start 15-60 us after the reset, and will last 60-240 us.
    // So 65us after the reset the bus must be high.
    GPIO_DIS_OUTPUT(ONEWIRE_PIN);
    os_delay_us(65);
    result = !GPIO_INPUT_GET(ONEWIRE_PIN);

    // After sending the reset pulse, the master (we) must wait at least another 480 us.
    os_delay_us(490);
    return result;
}

//
// Write a byte. The writing code uses the active drivers to raise the
// pin high, if you need power after the write (e.g. DS18S20 in
// parasite power mode) then set 'power' to 1, otherwise the pin will
// go tri-state at the end of the write to avoid heating in a short or
// other mishap.
//
void ICACHE_FLASH_ATTR onewire_write(uint8 v, uint32 power) {
    uint8 bitMask;
    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
        onewire_write_bit((bitMask & v) ? 1 : 0);
    }
    if (power) {
        GPIO_OUTPUT_SET(ONEWIRE_PIN, 1);
    }
}

//
// Write a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
void ICACHE_FLASH_ATTR onewire_write_bit(uint32 v) {
    GPIO_OUTPUT_SET(ONEWIRE_PIN, 0);
    if (v) {
        os_delay_us(10);
        GPIO_DIS_OUTPUT(ONEWIRE_PIN); // Drive output high using the pull-up
        os_delay_us(55);
    } else {
        os_delay_us(65);
        GPIO_DIS_OUTPUT(ONEWIRE_PIN); // Drive output high using the pull-up
        os_delay_us(5);
    }
}

//
// Read a byte
//
uint8 ICACHE_FLASH_ATTR onewire_read() {
    uint8 bitMask;
    uint8 r = 0;
    for (bitMask = 0x01; bitMask; bitMask <<= 1) {
        if (onewire_read_bit())
            r |= bitMask;
    }
    return r;
}

//
// Read a bit. Port and bit is used to cut lookup time and provide
// more certain timing.
//
uint32 ICACHE_FLASH_ATTR onewire_read_bit(void) {
    uint32 r;
    GPIO_OUTPUT_SET(ONEWIRE_PIN, 0);
    os_delay_us(3);
    GPIO_DIS_OUTPUT(ONEWIRE_PIN);
    os_delay_us(10);
    r = GPIO_INPUT_GET(ONEWIRE_PIN);
    os_delay_us(53);
    return r;
}

/* pass array of 8 bytes in */
uint32 ICACHE_FLASH_ATTR onewire_search(struct onewire_search_state *state) {

    // If last search returned the last device (no conflicts).
    if (state->lastDeviceFlag) {
        onewire_init_search_state(state);
        return FALSE;
    }

    // 1-Wire reset
    if (!onewire_reset()) {
        // Reset the search and return fault code.
        onewire_init_search_state(state);
        return ONEWIRE_SEARCH_NO_DEVICES;
    }

    // issue the search command
    onewire_write(ONEWIRE_SEARCH_ROM, 0);

    uint8 search_direction;
    int32 last_zero = -1;

    // Loop through all 8 bytes = 64 bits
    int32 id_bit_index;
    for (id_bit_index = 0; id_bit_index < 8 * ROM_BYTES; id_bit_index++) {
        const uint32 rom_byte_number = id_bit_index / BITS_PER_BYTE;
        const uint32 rom_byte_mask = 1 << (id_bit_index % BITS_PER_BYTE);

        // Read a bit and its complement
        const uint32 id_bit = onewire_read_bit();
        const uint32 cmp_id_bit = onewire_read_bit();

        // Line high for both reads means there are no slaves on the 1wire bus.
        if (id_bit == 1 && cmp_id_bit == 1) {
            // Reset the search and return fault code.
            onewire_init_search_state(state);
            return ONEWIRE_SEARCH_NO_DEVICES;
        }

        // No conflict for current bit: all devices coupled have 0 or 1
        if (id_bit != cmp_id_bit) {
            // Obviously, we continue the search using the same bit as all the devices have.
            search_direction = id_bit;
        } else {
            // if this discrepancy is before the Last Discrepancy
            // on a previous next then pick the same as last time
            if (id_bit_index < state->lastDiscrepancy) {
                search_direction = ((state->address[rom_byte_number]
                        & rom_byte_mask) > 0);
            } else {
                // if equal to last pick 1, if not then pick 0
                search_direction = (id_bit_index == state->lastDiscrepancy);
            }

            // if 0 was picked then record its position in LastZero
            if (search_direction == 0) {
                last_zero = id_bit_index;
            }
        }

        // set or clear the bit in the ROM byte rom_byte_number with mask rom_byte_mask
        if (search_direction == 1) {
            state->address[rom_byte_number] |= rom_byte_mask;
        } else {
            state->address[rom_byte_number] &= ~rom_byte_mask;
        }

        // For the current bit position, write the bit we choose to use in the current search.
        // Any devices that don't match are disabled.
        onewire_write_bit(search_direction);
    }

    state->lastDiscrepancy = last_zero;

    // check for last device
    if (state->lastDiscrepancy == -1) {
        state->lastDeviceFlag = TRUE;
    }

    if (crc8(state->address, 7) != state->address[7]) {
        // Reset the search and return fault code.
        onewire_init_search_state(state);
        return ONEWIRE_SEARCH_CRC_INVALID;
    }

    return ONEWIRE_SEARCH_FOUND;
}

//
// Do a ROM select
//
void ICACHE_FLASH_ATTR onewire_select(const uint8 *rom) {
    uint8 i = 0;
    onewire_write(ONEWIRE_MATCH_ROM, 0); // Choose ROM
    for (i = 0; i < 8; i++) {
        onewire_write(rom[i], 0);
    }
}

//
// Do a ROM skip
//
void ICACHE_FLASH_ATTR onewire_skip() {
    onewire_write(ONEWIRE_SKIP_ROM, 0); // Skip ROM
}

//
// Compute a Dallas Semiconductor 8 bit CRC directly.
// this is much slower, but much smaller, than the lookup table.
//
uint8 ICACHE_FLASH_ATTR crc8(const uint8 *addr, uint8 len) {
    uint8 crc = 0;
    uint8 i;
    while (len--) {
        uint8 inbyte = *addr++;
        for (i = 8; i; i--) {
            uint8 mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix)
                crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}
