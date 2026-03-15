/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include "user_settings/user_settings_secrets.h"

#define BOARD_TYPE_SEEED_XIAO_ESP32S3_SX1262 1
#define BOARD_TYPE_SEEED_XIAO_NRF52840_SX1262 2

namespace board
{
#if BOARD_TYPE == BOARD_TYPE_SEEED_XIAO_ESP32S3_SX1262
namespace hw
{
static constexpr uint32_t lora_cs = 41;            // GPIO4
static constexpr uint32_t lora_irq = 39;           // DIO1
static constexpr uint32_t lora_rst = 42;           // GPIO3
static constexpr uint32_t lora_gpio = 40;          // GPIO2
static constexpr uint32_t lora_led_green = GPIO_NUM_48;
static constexpr uint32_t lora_user_button = GPIO_NUM_21;
}
#elif BOARD_TYPE == BOARD_TYPE_SEEED_XIAO_NRF52840_SX1262
namespace hw
{
static constexpr uint32_t lora_cs = D4;
static constexpr uint32_t lora_irq = D1;
static constexpr uint32_t lora_rst = D2;
static constexpr uint32_t lora_gpio = D3;
static constexpr uint32_t lora_led_green = LED_GREEN;
static constexpr uint32_t lora_user_button = PIN_BUTTON1;
}
#else
#error Unsupported BOARD_TYPE
#endif
}

/** @brief Miscellaneous settings. */
#define DEBUG true
#define SERIAL_VERBOSITY 2

/** @brief Battery fit coefficients used to compute `VFIT` from `VGPIO`. */
#define VFIT_SLOPE 10.805f
#define VFIT_OFFSET -55.f
#define VBAT_MAX 4100.f
#define VBAT_MIN 3600.f
#define VBAT_NO_BATTERY_THRESHOLD (100.f * VFIT_SLOPE + VFIT_OFFSET)

/** @brief POSIX timezone string used by NTP/localtime; see https://github.com/nayarsystems/posix_tz_db for timezone values. */
#define NTP_TIMEZONE "CET-1CEST,M3.5.0/2,M10.5.0/3"

/** @brief LoRa settings. */
#define LORA_FREQ 868.0
#define LORA_BW 62.5
#define LORA_SF 12
#define LORA_CR 8
#define LORA_SYNCWORD RADIOLIB_SX126X_SYNC_WORD_PRIVATE
#define LORA_POWER 14
#define LORA_PREAMBLELENGTH 12
#define LORA_TCXOVOLTAGE 1.6
#define LORA_USEREGULATORLDO false

/** @brief MQTT settings. */
#define MQTT_ENABLED true
#define MQTT_SERVER "mqtt.ouilogique.ch"
#define MQTT_USE_TLS true
#define MQTT_PORT (MQTT_USE_TLS ? 8883 : 1883)
#define MQTT_TOPIC "mbx_rz"
#define MQTT_TOPIC_HEARTBEAT_RX "mbx_rz/heartbeat_rx"
#define MQTT_TOPIC_WAKEUP_PIN_HIGH "mbx_rz/pin"
#define MQTT_TOPIC_HEARTBEAT_TX "mbx_rz/heartbeat"
#define MQTT_TOPIC_BOOT "mbx_rz/boot"

/** @brief NTFY settings. */
#define NTFY_ENABLED true
#define NTFY_NOTIFY_HEARTBEAT_TX true
#define NTFY_ICON_PIN_HIGH "📩"
#define NTFY_ICON_HEARTBEAT_TX "🔔"
#define NTFY_SERVER "https://ntfy.ouilogique.ch/"
#define NTFY_RECIPIENT_NAME "rolf"
#define NTFY_TOPIC "mbx_rz"
