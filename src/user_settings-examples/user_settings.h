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
static constexpr bool TX_RESET_MSG_COUNTER_ON_REBOOT = false;
static constexpr uint32_t TX_HEARTBEAT_INTERVAL_MS = 60UL * 60UL * 1000UL;
static constexpr uint32_t RX_HEARTBEAT_INTERVAL_MS = 5000UL;
static constexpr uint32_t TX_DEBOUNCE_S = 5UL;
} // namespace settings::misc

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
} // namespace settings::board

/** @brief Battery fit coefficients used to compute battery voltage from analog input voltage. */
namespace settings::battery
{
static constexpr float FIT_SLOPE = 10.805f;
static constexpr float FIT_OFFSET = -55.f;
static constexpr float MAX = 4200.f;
static constexpr float MIN = 3600.f;
static constexpr float NO_BATTERY_THRESHOLD = 100.f * FIT_SLOPE + FIT_OFFSET;
} // namespace settings::battery

/** @brief POSIX timezone string used by NTP/localtime; see
 * https://github.com/nayarsystems/posix_tz_db for timezone values. */
namespace settings::ntp
{
static constexpr char TIMEZONE[] = "CET-1CEST,M3.5.0/2,M10.5.0/3";
static constexpr char SERVER[] = "pool.ntp.org";
static constexpr int SYNC_MAX_RETRIES = 10;
static constexpr uint32_t SYNC_RETRY_DELAY_MS = 500UL;
} // namespace settings::ntp

/** @brief Wi-Fi settings. */
namespace settings::wifi
{
static constexpr uint32_t CONNECT_TIMEOUT_MS = 10000UL;
static constexpr uint32_t CONNECT_RETRY_DELAY_MS = 500UL;
static constexpr uint32_t RECONNECT_MIN_INTERVAL_MS = 5000UL;
static constexpr uint32_t RECONNECT_TIMEOUT_MS = 8000UL;
} // namespace settings::wifi

/** @brief LoRa settings. */
namespace settings::lora
{
/** @brief Carrier frequency in MHz.
 * SX1262 capability: 150.0 to 960.0 MHz.
 * Legal/usable frequencies depend on the target region and local regulations.
 * Impact: changing this mainly changes regulatory compliance, propagation characteristics, and interference exposure.
 * Default in `SX1262.h`: `434.0`. */
static constexpr float FREQ = 868.0;

/** @brief LoRa bandwidth in kHz.
 * Typical values: 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500.
 * BW ↑ => throughput ↑, airtime ↓, sensitivity ↓, robustness in difficult environments ↓.
 * Default in `SX1262.h`: `125.0`. */
static constexpr float BW = 62.5;

/** @brief LoRa spreading factor. Valid range: 6 to 12.
 * Higher values improve sensitivity but reduce throughput.
 * SF ↑ => throughput ↓, airtime ↑, sensitivity ↑, range/penetration ↑, battery cost per message ↑.
 * Default in `SX1262.h`: `9`. */
static constexpr uint8_t SF = 12;

/** @brief LoRa coding rate denominator.
 * Valid range: 5 to 8.
 * CR ↑ => redundancy ↑, airtime ↑, throughput ↓, resilience to errors/interference ↑.
 * Default in `SX1262.h`: `7`. */
static constexpr uint8_t CR = 8;

/** @brief LoRa sync word used to distinguish networks.
 * `0x34` is reserved for LoRaWAN.
 * Impact: changes network separation only; it does not directly improve range, speed, or power use.
 * Default in `SX1262.h`: `RADIOLIB_SX126X_SYNC_WORD_PRIVATE`. */
static constexpr uint8_t SYNCWORD = RADIOLIB_SX126X_SYNC_WORD_PRIVATE;

/** @brief TX output power in dBm.
 * Typical SX1262 range: 2 to 17 dBm.
 * POWER ↑ => link budget ↑, range margin ↑, battery usage ↑, legal-risk/EMI risk ↑.
 * Default in `SX1262.h`: `10`. */
static constexpr int8_t POWER = 14;

/** @brief Preamble length in symbols.
 * The effective transmitted preamble is 4.25 symbols longer.
 * PREAMBLE_LENGTH ↑ => sync reliability ↑, airtime ↑, throughput ↓.
 * Default in `SX1262.h`: `8`. */
static constexpr uint16_t PREAMBLE_LENGTH = 12;

/** @brief TCXO control voltage in V.
 * Used by `radio.begin()` on boards that drive the SX1262 TCXO.
 * Impact: this is a hardware-matching parameter, not a tuning knob for speed or range.
 * Wrong value => startup failure, unstable RF behavior, or no radio link.
 * Default in `SX1262.h`: `1.6`. */
static constexpr float TCXO_VOLTAGE = 1.6;

/** @brief Use the SX1262 LDO regulator instead of DC-DC.
 * `false` is generally preferred for lower power.
 * USE_REGULATOR_LDO = true  => simplicity/compatibility may ↑, efficiency usually ↓.
 * USE_REGULATOR_LDO = false => efficiency usually ↑, but depends on board design and stability.
 * Default in `SX1262.h`: `false`. */
static constexpr bool USE_REGULATOR_LDO = false;
} // namespace settings::lora

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
} // namespace settings::mqtt

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
} // namespace settings::ntfy
