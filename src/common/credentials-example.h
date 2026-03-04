/*
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

/*
   Rename this file to credentials.h and update the details below with your information.
*/

#pragma once

#define CONFIG_ID 0

#if CONFIG_ID == 0

// Miscellaneous.
#define DEBUG true
#define SERIAL_VERBOSITY 2
#define HEARTBEATS_PER_DAY 86400 //  0..86400 (0 = disabled)

// Wifi.
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// MQTT.
/*
mosquitto_sub -h test.mosquitto.org -t "mbx-test"
mosquitto_pub -h test.mosquitto.org -t "mbx-test" -m '{"DATE":"'"$(date "+%Y-%m-%dT%H:%M:%S%z")"'"}'
*/
#define MQTT_ENABLED true // TODO implement MQTT_ENABLED
#define MQTT_SERVER "test.mosquitto.org"
#define MQTT_PORT 1883
#define MQTT_USE_TLS false
#define MQTT_TOPIC "mbx-test"
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

// NTFY.
/*
curl -fsSL \
    -H 'Title: Test with curl' \
    -d '{"DATE":"'"$(date "+%Y-%m-%dT%H:%M:%S%z")"'"}' \
    https://ntfy.sh/mbx-test
*/
#define NTFY_ENABLED true
#define NTFY_SERVER "https://ntfy.sh/"
#define NTFY_TOPIC "mbx-test"
#define NTFY_USERNAME ""
#define NTFY_PASSWORD ""

#elif CONFIG_ID == 1

/*
...
 */

 #endif
