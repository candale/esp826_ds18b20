#ifndef __USER_CONFIG_H_
#define __USER_CONFIG_H_

#define DEBUG_ON
// #define MQTT_DEBUG_ON

// WIFI CONFIGURATION
#define WIFI_SSID "ssid"
#define WIFI_PASS "pass"

// DEBUG OPTIONS
#if defined(DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

#endif
