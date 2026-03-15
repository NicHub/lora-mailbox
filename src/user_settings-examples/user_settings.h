/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include "user_settings/user_settings_secrets.h"

/** @brief Miscellaneous settings. */
#define DEBUG true
#define SERIAL_VERBOSITY 2

/** @brief Battery fit coefficients used to compute `VFIT` from `VGPIO`. */
#define VFIT_SLOPE 10.805f
#define VFIT_OFFSET -55.f
#define VGPIO_BATTERY_LOW_THRESHOLD 350

/** @brief POSIX timezone string used by NTP/localtime; see https://github.com/nayarsystems/posix_tz_db for timezone values. */
#define NTP_TIMEZONE "CET-1CEST,M3.5.0/2,M10.5.0/3"

/** @brief LoRa settings. */
#define LORA_FREQ 868.0
#define LORA_BW 62.5
#define LORA_SF 12
#define LORA_CR 8
#define LORA_SYNCWORD 0x12
#define LORA_POWER 14
#define LORA_PREAMBLELENGTH 12
#define LORA_TCXOVOLTAGE 1.6
#define LORA_USEREGULATORLDO false

/** @brief MQTT settings. */
#define MQTT_ENABLED true
#define MQTT_SERVER "mqtt.ouilogique.ch"
#define MQTT_USE_TLS true
#define MQTT_PORT (MQTT_USE_TLS ? 8883 : 1883)
#define MQTT_TOPIC "mbx_nj"
#define MQTT_TOPIC_HEARTBEAT_RX "mbx_nj/heartbeat_rx"
#define MQTT_TOPIC_WAKEUP_PIN_HIGH "mbx_nj/pin"
#define MQTT_TOPIC_HEARTBEAT_TX "mbx_nj/heartbeat"
#define MQTT_TOPIC_BOOT "mbx_nj/boot"
#define MQTT_TOPIC_LOW_BATTERY "mbx_nj/low_battery"

/** @brief NTFY settings. */
#define NTFY_ENABLED true
#define NTFY_INCLUDE_JSONL false
#define NTFY_NOTIFY_HEARTBEAT_TX true
#define NTFY_TITLE_PIN_HIGH "📩"
#define NTFY_TITLE_LOW_BATTERY "🪫"
#define NTFY_TITLE_PIN_HIGH_LOW_BATTERY "📩 + 🪫"
#define NTFY_TITLE_HEARTBEAT_TX "🔔"
#define NTFY_TITLE_HEARTBEAT_TX_LOW_BATTERY "🔔 + 🪫"
#define NTFY_SERVER "https://ntfy.ouilogique.ch/"
#define NTFY_RECIPIENT_NAME "nico"
#define NTFY_TOPIC "mbx_nj"
