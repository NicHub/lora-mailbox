/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <RadioLib.h>

#include "common/ntfy_types.h"
#include "user_settings/user_settings_secrets.h"

/** @brief Miscellaneous settings. */
namespace settings::misc
{
#define ROOT_TOPIC "mbx_rz"
    static constexpr char RECIPIENT_NAME[] = "rolf";
    static constexpr bool DEBUG = true;
    static constexpr int SERIAL_VERBOSITY = 2;
    static constexpr uint32_t TX_HEARTBEAT_INTERVAL_MS = 60UL * 60UL * 1000UL;
    static constexpr uint32_t TX_DEBOUNCE_S = 5UL;
    static constexpr uint32_t RX_HEARTBEAT_INTERVAL_MS = 5000UL;
}

namespace settings::board
{
#if BOARD_TYPE == 0
    /** @note ESP32S3 Wio-SX1262 pin mapping. */
    static constexpr uint32_t WAKEUP_PIN = D10;
    static constexpr uint32_t LORA_CS = 41;   // GPIO4
    static constexpr uint32_t LORA_IRQ = 39;  // DIO1
    static constexpr uint32_t LORA_RST = 42;  // GPIO3
    static constexpr uint32_t LORA_GPIO = 40; // GPIO2
    static constexpr uint32_t LORA_LED_GREEN = GPIO_NUM_48;
    static constexpr uint32_t LORA_USER_BUTTON = GPIO_NUM_21;
#elif BOARD_TYPE == 1
    /** @note XIAO nRF52840 Wio-SX1262 pin mapping. */
    static constexpr uint32_t WAKEUP_PIN = D5;
    static constexpr uint32_t LORA_CS = D4;
    static constexpr uint32_t LORA_IRQ = D1;
    static constexpr uint32_t LORA_RST = D2;
    static constexpr uint32_t LORA_GPIO = D3;
    static constexpr uint32_t LORA_LED_GREEN = LED_GREEN;
    static constexpr uint32_t LORA_USER_BUTTON = PIN_BUTTON1;
#else
#error Unsupported BOARD_TYPE
#endif
}

/** @brief Battery fit coefficients used to compute battery voltage from analog input voltage. */
namespace settings::battery
{
    static constexpr float FIT_SLOPE = 10.805f;
    static constexpr float FIT_OFFSET = -55.f;
    static constexpr float MAX = 4200.f;
    static constexpr float MIN = 3600.f;
    static constexpr float NO_BATTERY_THRESHOLD = 100.f * FIT_SLOPE + FIT_OFFSET;
}

/** @brief POSIX timezone string used by NTP/localtime; see
 * https://github.com/nayarsystems/posix_tz_db for timezone values. */
namespace settings::ntp
{
    static constexpr char TIMEZONE[] = "CET-1CEST,M3.5.0/2,M10.5.0/3";
    static constexpr char SERVER[] = "pool.ntp.org";
    static constexpr int SYNC_MAX_RETRIES = 10;
    static constexpr uint32_t SYNC_RETRY_DELAY_MS = 500UL;
}

/** @brief Wi-Fi settings. */
namespace settings::wifi
{
    static constexpr uint32_t CONNECT_TIMEOUT_MS = 10000UL;
    static constexpr uint32_t CONNECT_RETRY_DELAY_MS = 500UL;
    static constexpr uint32_t RECONNECT_MIN_INTERVAL_MS = 5000UL;
    static constexpr uint32_t RECONNECT_TIMEOUT_MS = 8000UL;
    /** @brief Delay before retrying after a reconnection attempt (covering all known APs) failed. */
    static constexpr uint32_t RECONNECT_RETRY_DELAY_MS = 30000UL;
}

/** @brief LoRa settings. */
namespace settings::lora
{
    static constexpr float FREQ = 868.0;
    static constexpr float BW = 62.5;
    static constexpr uint8_t SF = 12;
    static constexpr uint8_t CR = 8;
    static constexpr uint8_t SYNCWORD = RADIOLIB_SX126X_SYNC_WORD_PRIVATE;
    static constexpr int8_t POWER = 14;
    static constexpr uint16_t PREAMBLE_LENGTH = 12;
    static constexpr float TCXO_VOLTAGE = 1.6;
    static constexpr bool USE_REGULATOR_LDO = false;
}

/** @brief MQTT settings. */
namespace settings::mqtt
{
    static constexpr bool ENABLED = true;
    static constexpr char SERVER[] = "mqtt.ouilogique.ch";
    static constexpr bool USE_TLS = true;
    static constexpr int PORT = USE_TLS ? 8883 : 1883;
    static constexpr char TOPIC[] = ROOT_TOPIC;
    static constexpr char TOPIC_WAKE_BOOT[] = ROOT_TOPIC "/wake_boot";
    static constexpr char TOPIC_HEARTBEAT_TX[] = ROOT_TOPIC "/heartbeat_tx";
    static constexpr char TOPIC_HEARTBEAT_RX[] = ROOT_TOPIC "/heartbeat_rx";
    static constexpr char TOPIC_GOT_MAIL[] = ROOT_TOPIC "/got_mail";
}

/** @brief NTFY settings. */
namespace settings::ntfy
{
    static constexpr bool ENABLED = true;
    static constexpr bool NOTIFY_HEARTBEAT_TX = true;
    static constexpr char SERVER[] = "https://ntfy.ouilogique.ch/";
    static constexpr char TOPIC[] = ROOT_TOPIC;
    static constexpr char MD_CODE_TAG[] = "";

    static constexpr NTFYPriority MESSAGE_RECEIVED_PRIORITY = NTFYPriority::Max;
    static constexpr char MESSAGE_RECEIVED_ICON[] = "📩";
    static constexpr char MESSAGE_RECEIVED_TITLE_SUFFIX[] = " got mail";

    static constexpr NTFYPriority HEARTBEAT_PRIORITY = NTFYPriority::Min;
    static constexpr char HEARTBEAT_ICON[] = "🔔";
    static constexpr char HEARTBEAT_TITLE_SUFFIX[] = " got heartbeat";

    static constexpr NTFYPriority BOOT_PRIORITY = NTFYPriority::Default;
    static constexpr char BOOT_ICON[] = "🚀";
    static constexpr char BOOT_TITLE_SUFFIX[] = " boot";
}
