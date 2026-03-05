/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

/**
 * @brief Rename this file to `user_settings.h` and update values with your own configuration.
 */

#pragma once
#include <ArduinoJson.h>

#define CONFIG_ID 0

#if CONFIG_ID == 0

/** @brief Miscellaneous settings. */
#define DEBUG true
#define SERIAL_VERBOSITY 2
/** @brief Number of heartbeat messages per day (`0..86400`, `0` disables heartbeat). */
#define HEARTBEATS_PER_DAY 86400

/** @brief Wi-Fi credentials. */
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

/** @brief MQTT settings. */
/**
 * @code
 * mosquitto_sub -h test.mosquitto.org -t "mbx_test"
 * mosquitto_pub -h test.mosquitto.org -t "mbx_test" -m '{"DATE":"'"$(date "+%Y-%m-%dT%H:%M:%S%z")"'"}'
 * @endcode
 */
/** @brief Set to `false` to disable all MQTT publications. */
#define MQTT_ENABLED true
#define MQTT_SERVER "test.mosquitto.org"
#define MQTT_PORT 1883
#define MQTT_USE_TLS false
#define MQTT_TOPIC "mbx_test"
#define MQTT_TOPIC_HEARTBEAT_RX "mbx_test/heartbeat_rx"
#define MQTT_TOPIC_WAKEUP_PIN_HIGH "mbx_test/pin"
#define MQTT_TOPIC_HEARTBEAT_TX "mbx_test/heartbeat"
#define MQTT_TOPIC_BOOT "mbx_test/boot"
#define MQTT_BATTERY_LOW_THRESHOLD_MV 320
#define MQTT_TOPIC_LOW_BATTERY "mbx_test/low_battery"
/**
 * @brief Optional hook for extra MQTT topic routing constraints.
 * @return A topic string, or `nullptr` / empty string when no custom rule matches.
 */
/**
 * @code
 * static inline const char *mqttCustomTopicResolver(
 *     const JsonDocument &jsonDoc,
 *     const char *wakeup,
 *     uint16_t batteryMv)
 * {
 *     if ((jsonDoc["cnt"] | 0) % 10 == 0)
 *         return "mbx_test/every_10";
 *     return nullptr;
 * }
 * #define MQTT_CUSTOM_TOPIC_RESOLVER mqttCustomTopicResolver
 * @endcode
 */
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

/** @brief NTFY settings. */
/**
 * @code
 * curl -fsSL \
 *     -H 'Title: Test with curl' \
 *     -d '{"DATE":"'"$(date "+%Y-%m-%dT%H:%M:%S%z")"'"}' \
 *     https://ntfy.sh/mbx_test
 * @endcode
 */
/** @brief Set to `false` to disable all NTFY notifications. */
#define NTFY_ENABLED false
#define NTFY_SERVER "https://ntfy.sh/"
#define NTFY_TOPIC "mbx_test"
#define NTFY_USERNAME ""
#define NTFY_PASSWORD ""

#elif CONFIG_ID == 1

/** @brief Miscellaneous settings. */
#define DEBUG true
#define SERIAL_VERBOSITY 2
/** @brief Number of heartbeat messages per day (`0..86400`, `0` disables heartbeat). */
#define HEARTBEATS_PER_DAY 86400

/** @brief Wi-Fi credentials. */
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

/** @brief MQTT settings. */
/**
 * @code
 * mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_nj"
 * mosquitto_sub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_nj/#" -v
 * mosquitto_pub -h mqtt.ouilogique.ch -p 8883 -u guest -P guest123 -t "mbx_nj" -m '{"DATE":"'"$(date "+%Y-%m-%dT%H:%M:%S%z")"'"}'
 * @endcode
 */
/** @brief Set to `false` to disable all MQTT publications. */
#define MQTT_ENABLED true
#define MQTT_SERVER "mqtt.ouilogique.ch"
#define MQTT_PORT 8883
#define MQTT_USE_TLS true
#define MQTT_TOPIC "mbx_nj"
#define MQTT_TOPIC_HEARTBEAT_RX "mbx_nj/heartbeat_rx"
#define MQTT_TOPIC_WAKEUP_PIN_HIGH "mbx_nj/pin"
#define MQTT_TOPIC_HEARTBEAT_TX "mbx_nj/heartbeat"
#define MQTT_TOPIC_BOOT "mbx_nj/boot"
#define MQTT_BATTERY_LOW_THRESHOLD_MV 320
#define MQTT_TOPIC_LOW_BATTERY "mbx_nj/low_battery"
/**
 * @brief Optional hook for extra MQTT topic routing constraints.
 * @return A topic string, or `nullptr` / empty string when no custom rule matches.
 */
/**
 * @code
 * static inline const char *mqttCustomTopicResolver(
 *     const JsonDocument &jsonDoc,
 *     const char *wakeup,
 *     uint16_t batteryMv)
 * {
 *     if ((jsonDoc["cnt"] | 0) % 10 == 0)
 *         return "mbx_nj/every_10";
 *     return nullptr;
 * }
 * #define MQTT_CUSTOM_TOPIC_RESOLVER mqttCustomTopicResolver
 * @endcode
 */
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

/** @brief NTFY settings. */
/**
 * @code
 * curl -fsSL \
 *     -u USERNAME:PASSWORD \
 *     -H 'Title: Test with curl' \
 *     -d '{"DATE":"'"$(date "+%Y-%m-%dT%H:%M:%S%z")"'"}' \
 *     https://ntfy.ouilogique.ch/mbx_nj
 * @endcode
 */
/** @brief Set to `false` to disable all NTFY notifications. */
#define NTFY_ENABLED false
#define NTFY_SERVER "https://ntfy.ouilogique.ch/"
#define NTFY_TOPIC "mbx_nj"
#define NTFY_USERNAME ""
#define NTFY_PASSWORD ""

#endif
