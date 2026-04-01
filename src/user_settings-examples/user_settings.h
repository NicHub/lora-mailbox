/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include "common/LoraMailBox_NTFY_types.h"
#include "user_settings/user_settings_secrets.h"

namespace settings::mailbox
{
    static constexpr char recipient_name[] = "rolf";
}

namespace board
{
    static constexpr int seeed_xiao_esp32s3_sx1262 = 1;
    static constexpr int seeed_xiao_nrf52840_sx1262 = 2;

#if BOARD_TYPE == 1
    namespace hw
    {
        static constexpr uint32_t wakeup_pin = D10;
        static constexpr uint32_t lora_cs = 41;   // GPIO4
        static constexpr uint32_t lora_irq = 39;  // DIO1
        static constexpr uint32_t lora_rst = 42;  // GPIO3
        static constexpr uint32_t lora_gpio = 40; // GPIO2
        static constexpr uint32_t lora_led_green = GPIO_NUM_48;
        static constexpr uint32_t lora_user_button = GPIO_NUM_21;
    }
#elif BOARD_TYPE == 2
    namespace hw
    {
        static constexpr uint32_t wakeup_pin = D5;
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
static constexpr bool DEBUG = true;
static constexpr int SERIAL_VERBOSITY = 2;
static constexpr bool TX_RESET_MSG_COUNTER_ON_REBOOT = false;

/** @brief Battery fit coefficients used to compute `VFIT` from `VGPIO`. */
static constexpr float VFIT_SLOPE = 10.805f;
static constexpr float VFIT_OFFSET = -55.f;
static constexpr float VBAT_MAX = 4100.f;
static constexpr float VBAT_MIN = 3600.f;
static constexpr float VBAT_NO_BATTERY_THRESHOLD = 100.f * VFIT_SLOPE + VFIT_OFFSET;

/** @brief POSIX timezone string used by NTP/localtime; see https://github.com/nayarsystems/posix_tz_db for timezone values. */
static constexpr char NTP_TIMEZONE[] = "CET-1CEST,M3.5.0/2,M10.5.0/3";
static constexpr char NTP_SERVER[] = "pool.ntp.org";
static constexpr int NTP_SYNC_MAX_RETRIES = 10;
static constexpr unsigned long NTP_SYNC_RETRY_DELAY_MS = 500UL;

/** @brief Wi-Fi settings. */
static constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 10000UL;
static constexpr unsigned long WIFI_CONNECT_RETRY_DELAY_MS = 500UL;
static constexpr unsigned long WIFI_RECONNECT_MIN_INTERVAL_MS = 5000UL;
static constexpr unsigned long WIFI_RECONNECT_TIMEOUT_MS = 8000UL;

/** @brief LoRa settings. */
static constexpr float LORA_FREQ = 868.0;
static constexpr float LORA_BW = 62.5;
static constexpr uint8_t LORA_SF = 12;
static constexpr uint8_t LORA_CR = 8;
static constexpr uint8_t LORA_SYNCWORD = RADIOLIB_SX126X_SYNC_WORD_PRIVATE;
static constexpr int8_t LORA_POWER = 14;
static constexpr uint16_t LORA_PREAMBLELENGTH = 12;
static constexpr float LORA_TCXOVOLTAGE = 1.6;
static constexpr bool LORA_USEREGULATORLDO = false;
// 868.0 MHz: respect legal duty-cycle constraints (typically 1%), so avoid short intervals.
static constexpr unsigned long HEARTBEAT_INTERVAL_SECONDS = 60UL * 60UL;
static constexpr unsigned long HEARTBEAT_INTERVAL_MS = HEARTBEAT_INTERVAL_SECONDS * 1000UL;
static constexpr unsigned long HEARTBEAT_RX_INTERVAL_MS = 5000UL;
static constexpr unsigned long TX_DEBOUNCE_S = 5UL;

/** @brief MQTT settings. */
static constexpr bool MQTT_ENABLED = true;
static constexpr char MQTT_SERVER[] = "mqtt.ouilogique.ch";
static constexpr bool MQTT_USE_TLS = true;
static constexpr int MQTT_PORT = MQTT_USE_TLS ? 8883 : 1883;
static constexpr char MQTT_TOPIC[] = "mbx_rz";
static constexpr char MQTT_TOPIC_HEARTBEAT_RX[] = "mbx_rz/heartbeat_rx";
static constexpr char MQTT_TOPIC_WAKEUP_PIN_HIGH[] = "mbx_rz/pin";
static constexpr char MQTT_TOPIC_HEARTBEAT_TX[] = "mbx_rz/heartbeat";
static constexpr char MQTT_TOPIC_BOOT[] = "mbx_rz/boot";
static constexpr char SIMULATOR_MQTT_BASE_TOPIC[] = "mbx_rz/simulator";
static constexpr char SIMULATOR_MQTT_ON_TOPIC[] = "mbx_rz/simulator/on";
static constexpr char SIMULATOR_MQTT_OFF_TOPIC[] = "mbx_rz/simulator/off";
static constexpr char SIMULATOR_MQTT_PWM_TOPIC[] = "mbx_rz/simulator/pwm";
static constexpr char SIMULATOR_MQTT_STATUS_TOPIC[] = "mbx_rz/simulator/status";

/** @brief NTFY settings. */
static constexpr bool NTFY_ENABLED = true;
namespace settings::ntfy
{
    static constexpr bool notify_heartbeat_tx = true;
    static constexpr char server[] = "https://ntfy.ouilogique.ch/";
    static constexpr char topic[] = "mbx_rz";

    static constexpr NTFYPriority message_received_priority = NTFYPriority::Max;
    static constexpr char message_received_icon[] = "📩";
    static constexpr char message_received_title_suffix[] = " got mail";

    static constexpr NTFYPriority heartbeat_priority = NTFYPriority::Min;
    static constexpr char heartbeat_icon[] = "🔔";
    static constexpr char heartbeat_title_suffix[] = " got heartbeat";
}
