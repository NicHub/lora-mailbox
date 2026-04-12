/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <RadioLib.h>

#include "common/LoraMailBox_NTFY_types.h"
#include "user_settings/user_settings_secrets.h"

/** @brief Miscellaneous settings. */
namespace settings::misc
{
#define ROOT_TOPIC "mbx_rz"
    static constexpr char recipient_name[] = "rolf";
    static constexpr bool debug = true;
    static constexpr int serial_verbosity = 2;
    static constexpr bool tx_reset_msg_counter_on_reboot = false;
}

namespace settings::board
{
#if BOARD_TYPE == 0
    /** @note ESP32S3 Wio-SX1262 pin mapping. */
    static constexpr uint32_t wakeup_pin = D10;
    static constexpr uint32_t lora_cs = 41;   // GPIO4
    static constexpr uint32_t lora_irq = 39;  // DIO1
    static constexpr uint32_t lora_rst = 42;  // GPIO3
    static constexpr uint32_t lora_gpio = 40; // GPIO2
    static constexpr uint32_t lora_led_green = GPIO_NUM_48;
    static constexpr uint32_t lora_user_button = GPIO_NUM_21;
#elif BOARD_TYPE == 1
    /** @note XIAO nRF52840 Wio-SX1262 pin mapping. */
    static constexpr uint32_t wakeup_pin = D5;
    static constexpr uint32_t lora_cs = D4;
    static constexpr uint32_t lora_irq = D1;
    static constexpr uint32_t lora_rst = D2;
    static constexpr uint32_t lora_gpio = D3;
    static constexpr uint32_t lora_led_green = LED_GREEN;
    static constexpr uint32_t lora_user_button = PIN_BUTTON1;
#else
#error Unsupported BOARD_TYPE
#endif
}

/** @brief Battery fit coefficients used to compute battery voltage from analog input voltage. */
namespace settings::battery
{
    static constexpr float fit_slope = 10.805f;
    static constexpr float fit_offset = -55.f;
    static constexpr float max = 4100.f;
    static constexpr float min = 3600.f;
    static constexpr float no_battery_threshold = 100.f * fit_slope + fit_offset;
}

/** @brief POSIX timezone string used by NTP/localtime; see https://github.com/nayarsystems/posix_tz_db for timezone values. */
namespace settings::ntp
{
    static constexpr char timezone[] = "CET-1CEST,M3.5.0/2,M10.5.0/3";
    static constexpr char server[] = "pool.ntp.org";
    static constexpr int sync_max_retries = 10;
    static constexpr unsigned long sync_retry_delay_ms = 500UL;
}

/** @brief Wi-Fi settings. */
namespace settings::wifi
{
    static constexpr unsigned long connect_timeout_ms = 10000UL;
    static constexpr unsigned long connect_retry_delay_ms = 500UL;
    static constexpr unsigned long reconnect_min_interval_ms = 5000UL;
    static constexpr unsigned long reconnect_timeout_ms = 8000UL;
}

/** @brief LoRa settings. */
namespace settings::lora
{
    static constexpr float freq = 868.0;
    static constexpr float bw = 62.5;
    static constexpr uint8_t sf = 12;
    static constexpr uint8_t cr = 8;
    static constexpr uint8_t syncword = RADIOLIB_SX126X_SYNC_WORD_PRIVATE;
    static constexpr int8_t power = 14;
    static constexpr uint16_t preamble_length = 12;
    static constexpr float tcxo_voltage = 1.6;
    static constexpr bool use_regulator_ldo = false;
    static constexpr unsigned long tx_heartbeat_interval_ms = 60UL * 60UL * 1000UL;
    static constexpr unsigned long rx_heartbeat_interval_ms = 5000UL;
    static constexpr unsigned long tx_debounce_s = 5UL;
}

/** @brief MQTT settings. */
namespace settings::mqtt
{
    static constexpr bool enabled = true;
    static constexpr char server[] = "mqtt.ouilogique.ch";
    static constexpr bool use_tls = true;
    static constexpr int port = use_tls ? 8883 : 1883;
    static constexpr char topic[] = ROOT_TOPIC;
    static constexpr char topic_wake_boot[] = ROOT_TOPIC "/wake_boot";
    static constexpr char topic_heartbeat_tx[] = ROOT_TOPIC "/heartbeat_tx";
    static constexpr char topic_heartbeat_rx[] = ROOT_TOPIC "/heartbeat_rx";
    static constexpr char topic_got_mail[] = ROOT_TOPIC "/got_mail";
}

/** @brief NTFY settings. */
namespace settings::ntfy
{
    static constexpr bool enabled = true;
    static constexpr bool notify_heartbeat_tx = true;
    static constexpr char server[] = "https://ntfy.ouilogique.ch/";
    static constexpr char topic[] = ROOT_TOPIC;

    static constexpr NTFYPriority message_received_priority = NTFYPriority::Max;
    static constexpr char message_received_icon[] = "📩";
    static constexpr char message_received_title_suffix[] = " got mail";

    static constexpr NTFYPriority heartbeat_priority = NTFYPriority::Min;
    static constexpr char heartbeat_icon[] = "🔔";
    static constexpr char heartbeat_title_suffix[] = " got heartbeat";
}
