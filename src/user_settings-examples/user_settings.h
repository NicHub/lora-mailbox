/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <RadioLib.h>
#include <stddef.h>
#include <string.h>

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

/** @brief Wi-Fi settings:
 * CONNECTION_PRIORITY selects whether RX prefers the strongest configured SSID
 * currently visible (`SignalStrength`) or simply tries the configured SSIDs in
 * declaration order (`SettingsOrder`).
 * CONNECT_TIMEOUT_MS is the maximum duration of one connection attempt,
 * whether during boot or after a disconnect.
 * RETRY_DELAY_MS is the pause before a new attempt after a failure.
 */
namespace settings::wifi
{
    enum class ConnectionPriority : uint8_t
    {
        SignalStrength,
        SettingsOrder,
    };
    static constexpr ConnectionPriority CONNECTION_PRIORITY = ConnectionPriority::SettingsOrder;
    static constexpr uint32_t CONNECT_TIMEOUT_MS = 10000UL;
    static constexpr uint32_t RETRY_DELAY_MS = 30000UL;
}

/** @brief LoRa settings. */
namespace settings::lora
{
    struct Parameters
    {
        const char *name;
        float freq;
        float bw;
        uint8_t sf;
        uint8_t cr;
        uint8_t syncword;
        int8_t power;
        uint16_t preamble_length;
        float tcxo_voltage;
        bool use_regulator_ldo;
    };

    static constexpr Parameters LORA_PROFILES[] = {
        {
            "faraday",
            868.0,
            62.5,
            12,
            8,
            RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
            14,
            12,
            1.6,
            false,
        },
        {
            "fast",
            868.0,
            500.0,
            5,
            4,
            RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
            14,
            8,
            1.6,
            false,
        },
        {
            "sx1262_default",
            434.0,
            125.0,
            9,
            7,
            RADIOLIB_SX126X_SYNC_WORD_PRIVATE,
            10,
            8,
            1.6,
            false,
        },
    };

    static constexpr size_t LORA_PROFILE_COUNT = sizeof(LORA_PROFILES) / sizeof(LORA_PROFILES[0]);
    static constexpr size_t DEFAULT_LORA_PROFILE_INDEX = 1;
    static_assert(LORA_PROFILE_COUNT > 0, "At least one LoRa profile must be defined");
    static_assert(DEFAULT_LORA_PROFILE_INDEX < LORA_PROFILE_COUNT, "Invalid default LoRa profile index");

    namespace detail
    {
        extern size_t current_lora_profile_index;
    }

    static inline bool isValidLoraProfileIndex(size_t index)
    {
        return index < LORA_PROFILE_COUNT;
    }

    static inline bool findLoraProfileIndexByName(const char *name, size_t &index)
    {
        if (name == nullptr)
            return false;

        for (size_t i = 0; i < LORA_PROFILE_COUNT; ++i)
        {
            if (strcmp(LORA_PROFILES[i].name, name) == 0)
            {
                index = i;
                return true;
            }
        }
        return false;
    }

    static inline bool isValidLoraProfileName(const char *name)
    {
        size_t index = 0;
        return findLoraProfileIndexByName(name, index);
    }

    static inline size_t getLoraProfileIndex()
    {
        return detail::current_lora_profile_index;
    }

    static inline bool setLoraProfileIndex(size_t index)
    {
        if (!isValidLoraProfileIndex(index))
            return false;
        detail::current_lora_profile_index = index;
        return true;
    }

    static inline bool setLoraProfileName(const char *name)
    {
        size_t index = 0;
        if (!findLoraProfileIndexByName(name, index))
            return false;
        return setLoraProfileIndex(index);
    }

    static inline const Parameters &loraProfile(size_t index)
    {
        return LORA_PROFILES[isValidLoraProfileIndex(index) ? index : DEFAULT_LORA_PROFILE_INDEX];
    }

    static inline const Parameters &loraProfile(const char *name)
    {
        size_t index = 0;
        return loraProfile(findLoraProfileIndexByName(name, index) ? index : DEFAULT_LORA_PROFILE_INDEX);
    }

    static inline const Parameters &current()
    {
        return loraProfile(detail::current_lora_profile_index);
    }

    static inline const char *getLoraProfileName()
    {
        return current().name;
    }
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
    static constexpr bool NOTIFY_WIFI_RECONNECTED = true;
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

    static constexpr NTFYPriority WIFI_RECONNECTED_PRIORITY = NTFYPriority::High;
    static constexpr char WIFI_RECONNECTED_ICON[] = "📶";
    static constexpr char WIFI_RECONNECTED_TITLE_SUFFIX[] = " wifi reconnected";
}
