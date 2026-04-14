/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include "RX_ESP32.h"
#include "common/common.h"
#include "wifi.h"
#include "mqtt.h"
#include "ntfy.h"

LoraMailboxWifi lmb_wifi;
LoraMailboxMqtt lmb_mqtt;
LoraMailboxNtfy lmb_ntfy;
String json_string;
JsonDocument json_doc;

static String bytesToHex(const String &input)
{
    static constexpr char HEX_DIGITS[] = "0123456789abcdef";
    String hex;
    hex.reserve(input.length() * 2);
    for (size_t i = 0; i < input.length(); ++i)
    {
        const uint8_t byte_value = static_cast<uint8_t>(input[i]);
        hex += HEX_DIGITS[byte_value >> 4];
        hex += HEX_DIGITS[byte_value & 0x0F];
    }
    return hex;
}

/**
 * @brief Return whether the current payload reports a mailbox event.
 * @return true when the TX trigger equals `WAKEUP_PIN_HIGH`, false otherwise.
 */
bool isPinHighEvent()
{
    const char *trigger = json_doc["TX"]["TX_TRIGGER"] | "";
    if (trigger[0] == '\0')
        return false;
    return strcmp(trigger, "WAKEUP_PIN_HIGH") == 0;
}

/**
 * @brief Return whether the current payload reports a TX heartbeat event.
 * @return true when the TX trigger equals `HEARTBEAT_TX`, false otherwise.
 */
bool isHeartbeatTxEvent()
{
    const char *trigger = json_doc["TX"]["TX_TRIGGER"] | "";
    if (trigger[0] == '\0')
        return false;
    return strcmp(trigger, "HEARTBEAT_TX") == 0;
}

/**
 * @brief Return whether the current payload reports a TX boot event.
 * @return true when the TX trigger equals `BOOT`, false otherwise.
 */
bool isBootEvent()
{
    const char *trigger = json_doc["TX"]["TX_TRIGGER"] | "";
    if (trigger[0] == '\0')
        return false;
    return strcmp(trigger, "BOOT") == 0;
}

/**
 * @brief Forward the current payload to NTFY when it matches notification rules.
 */
void broadcastNtfy()
{
    bool should_notify_ntfy = isPinHighEvent();
    should_notify_ntfy =
        should_notify_ntfy || (settings::ntfy::NOTIFY_HEARTBEAT_TX && isHeartbeatTxEvent());
    should_notify_ntfy = should_notify_ntfy || isBootEvent();
    if (should_notify_ntfy)
        lmb_ntfy.sendMsg(json_doc);
}

void broadcastResults()
{
    serializeJson(json_doc, json_string);
    lmb_wifi.sendMsg(json_string);
    lmb_mqtt.sendMsg(json_doc);
    broadcastNtfy();
    if (settings::misc::SERIAL_VERBOSITY == 1)
    {
        uint16_t ctr = json_doc["RX"]["RX_COUNTER"] | 0;
        static uint16_t first_ctr = ctr;
        ctr -= first_ctr;

        uint32_t ms = millis();
        static uint32_t first_ms = ms;
        ms -= first_ms;

        Serial.printf("- ctr: %3u , ms: %8u\n", ctr, ms);
    }
    else if (settings::misc::SERIAL_VERBOSITY == 2)
    {
        Serial.println(json_string);
    }
}

void counterCheck()
{
    if (json_doc["TX"]["TX_COUNTER"].isNull())
        return;
    uint16_t tx_cnt = json_doc["TX"]["TX_COUNTER"].as<uint16_t>();
    static uint16_t prev_tx_cnt = tx_cnt - 1;
    static uint16_t error_count = 0;
    uint16_t rx_cnt = prev_tx_cnt + 1;
    bool counter_status = tx_cnt != rx_cnt;
    error_count += counter_status;

    json_doc["RX"]["RX_COUNTER"] = rx_cnt;
    json_doc["RX_TX"]["RX_TX_COUNTER_ERROR_COUNT"] = error_count;
    json_doc["RX_TX"]["RX_TX_COUNTER_STATUS"] = counter_status ? "NOT OK" : "OK";

    prev_tx_cnt = tx_cnt;
}

const char *getCurrentTime()
{
    time_t now = time(nullptr);
    struct tm local_tm = *localtime(&now);
    struct tm utc_tm = *gmtime(&now);
    int offset_min = (local_tm.tm_hour - utc_tm.tm_hour) * 60 + (local_tm.tm_min - utc_tm.tm_min);
    if (local_tm.tm_mday != utc_tm.tm_mday)
        offset_min += (local_tm.tm_hour < 12) ? 1440 : -1440;
    int offset_h = offset_min / 60;
    int offset_m = abs(offset_min % 60);
    static char time_str[26];
    snprintf(
        time_str,
        sizeof(time_str),
        "%04d-%02d-%02dT%02d:%02d:%02d%+03d:%02d",
        local_tm.tm_year + 1900,
        local_tm.tm_mon + 1,
        local_tm.tm_mday,
        local_tm.tm_hour,
        local_tm.tm_min,
        local_tm.tm_sec,
        offset_h,
        offset_m);
    return time_str;
}

void addLoraSettingsToJsonDoc()
{
    json_doc["LORA"]["LORA_FREQ"] = settings::lora::FREQ;
    json_doc["LORA"]["LORA_BW"] = settings::lora::BW;
    json_doc["LORA"]["LORA_SF"] = settings::lora::SF;
    json_doc["LORA"]["LORA_CR"] = settings::lora::CR;
    json_doc["LORA"]["LORA_SYNCWORD"] = settings::lora::SYNCWORD;
    json_doc["LORA"]["LORA_POWER"] = settings::lora::POWER;
    json_doc["LORA"]["LORA_PREAMBLE_LENGTH"] = settings::lora::PREAMBLE_LENGTH;
    json_doc["LORA"]["LORA_TCXO_VOLTAGE"] = settings::lora::TCXO_VOLTAGE;
    json_doc["LORA"]["LORA_USE_REGULATOR_LDO"] = settings::lora::USE_REGULATOR_LDO;
}

void heartBeat()
{
    uint32_t now_ms = millis();
    static uint32_t prev_heart_beat = now_ms;
    if (now_ms - prev_heart_beat < settings::misc::RX_HEARTBEAT_INTERVAL_MS)
        return;
    prev_heart_beat = now_ms;

    json_doc.clear();
    json_doc["RX"]["RX_BOARD_ID"] = getMacAddress();
    json_doc["RX"]["RX_CURRENT_LOCAL_TIME"] = getCurrentTime();
    json_doc["RX"]["RX_BUILD_LOCAL_TIME"] = BUILD_LOCAL_TIME;
    json_doc["RX"]["RX_GIT_HEAD_COMMIT_ID"] = GIT_HEAD_COMMIT_ID;
    json_doc["RX"]["RX_GIT_UNCOMMITTED_FILES_COUNT"] = GIT_UNCOMMITTED_FILES_COUNT;
    json_doc["RX"]["RX_TRIGGER"] = "HEARTBEAT_RX";
    json_doc["RX"]["RX_WEB_UI_URL"] = String("http://") + lmb_wifi.getLocalIP().toString();

    serializeJson(json_doc, json_string);
    if (settings::misc::SERIAL_VERBOSITY == 2)
        Serial.println(json_string);
    lmb_wifi.sendMsg(json_string);
    lmb_mqtt.sendMsg(json_doc);
}

void readLoRa()
{
    digitalWrite(settings::board::LORA_LED_GREEN, HIGH);
    radio.startReceive();
    int state = radio.readData(json_string);
    digitalWrite(settings::board::LORA_LED_GREEN, LOW);

    json_doc["RX_LORA_STATE"] = state;
    if (state != RADIOLIB_ERR_NONE)
        return;
    JsonDocument tx_doc;
    DeserializationError deserialize_error = deserializeJson(tx_doc, json_string);
    json_doc.clear();
    JsonObject tx = json_doc["TX"].to<JsonObject>();
    if (deserialize_error)
    {
        tx["TX_JSON_DESERIALIZE_ERROR"] = deserialize_error.c_str();
        tx["TX_JSON_STRING_HEX"] = bytesToHex(json_string);
    }
    else
    {
        for (JsonPairConst kv : tx_doc.as<JsonObjectConst>())
            tx[kv.key()] = kv.value();
        tx["TX_JSON_STRING"] = json_string;
    }
    json_doc["RX"]["RX_BOARD_ID"] = getMacAddress();
    json_doc["RX"]["RX_BUILD_LOCAL_TIME"] = BUILD_LOCAL_TIME;
    json_doc["RX"]["RX_CURRENT_LOCAL_TIME"] = getCurrentTime();
    json_doc["RX"]["RX_DEBUG"] = settings::misc::DEBUG;
    json_doc["RX"]["RX_GIT_HEAD_COMMIT_ID"] = GIT_HEAD_COMMIT_ID;
    json_doc["RX"]["RX_GIT_UNCOMMITTED_FILES_COUNT"] = GIT_UNCOMMITTED_FILES_COUNT;
    json_doc["RX"]["RX_TRIGGER"] = "LORA_PAYLOAD_RECEIVED";
    json_doc["RX"]["RX_WEB_UI_URL"] = String("http://") + lmb_wifi.getLocalIP().toString();
    json_doc["RX"]["RX_WS_CLIENT_COUNT"] = lmb_wifi.getWsClientCount();
    addLoraSettingsToJsonDoc();
    json_doc["RX_TX"]["RX_TX_RSSI_DBM"] = radio.getRSSI();
    json_doc["RX_TX"]["RX_TX_SNR_DB"] = radio.getSNR();
    if (!json_doc["TX"]["TX_VBAT_RAW"].isNull())
    {
        BatteryMeasurement battery = vbatRaw2VbatMv(json_doc["TX"]["TX_VBAT_RAW"].as<uint16_t>());
        json_doc["TX"]["TX_VBAT_MV"] = battery.vbat_mv;
        json_doc["TX"]["TX_VBAT_PERCENT"] = battery.battery_percent;
        json_doc["TX"]["TX_VBAT_GLYPH"] = batteryGlyphToString(battery.glyph);
        json_doc["TX"]["TX_VBAT_STATUS"] = batteryStatusToString(battery.status);
    }
}

void setupMQTT()
{
    lmb_mqtt.begin();
    lmb_mqtt.sendMsg(json_doc);
}

void setupWiFi()
{
    lmb_wifi.begin();
    lmb_wifi.synchronizeNTPTime();
}

void setupLoRaRX()
{
    radio.setDio1Action(setFlag);
    radio.startReceive();
}

/**
 * @brief Configure GPIO directions for runtime peripherals.
 */
void setupGPIOs()
{
    /**
     * @note On XIAO ESP32S3 with SX1262 shield, `settings::board::LORA_USER_BUTTON` and
     * `LED_BUILTIN` share GPIO21. Driving `LED_BUILTIN` high while pressing
     * the user button can create a short-circuit to GND.
     * @note Keep `pinMode(LED_BUILTIN, OUTPUT);` disabled for safety.
     */
    pinMode(settings::board::LORA_LED_GREEN, OUTPUT);
}

void setup()
{
    setupGPIOs();
    blink(8UL, 200UL, 5UL, settings::board::LORA_LED_GREEN, false);
    setupSerial();
    setupLoRa();
    setupLoRaRX();
    setupWiFi();
    setupMQTT();
}

void loop()
{
    statusLed.update();
    lmb_wifi.ensureWiFiConnected();
    heartBeat();
    yield();
    if (!loraEvent)
        return;
    loraEvent = false;
    readLoRa();
    counterCheck();
    broadcastResults();
    statusLed.start(8UL, 60UL, 10UL, settings::board::LORA_LED_GREEN, false);
}
