/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include "common/common_ESP32.h"
#include "common/common.h"
#include "LoraMailBox_WIFI.h"
#include "LoraMailBox_MQTT.h"
#include "LoraMailBox_NTFY.h"

LoraMailBox_WIFI lmb_wifi;
LoraMailBox_MQTT lmb_mqtt;
LoraMailBox_NTFY lmb_ntfy;
String jsonString;
JsonDocument jsonDoc;

static String bytesToHex(const String &input)
{
    static constexpr char HEX_DIGITS[] = "0123456789abcdef";
    String hex;
    hex.reserve(input.length() * 2);
    for (size_t i = 0; i < input.length(); ++i)
    {
        const uint8_t byteValue = static_cast<uint8_t>(input[i]);
        hex += HEX_DIGITS[byteValue >> 4];
        hex += HEX_DIGITS[byteValue & 0x0F];
    }
    return hex;
}

/**
 * @brief Return whether the current payload reports a mailbox event.
 * @return true when the TX trigger equals `WAKEUP_PIN_HIGH`, false otherwise.
 */
bool isPinHighEvent()
{
    const char *trigger = jsonDoc["TX"]["TX_TRIGGER"] | "";
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
    const char *trigger = jsonDoc["TX"]["TX_TRIGGER"] | "";
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
    const char *trigger = jsonDoc["TX"]["TX_TRIGGER"] | "";
    if (trigger[0] == '\0')
        return false;
    return strcmp(trigger, "BOOT") == 0;
}

/**
 * @brief Forward the current payload to NTFY when it matches notification rules.
 */
void broadcastNtfy()
{
    bool shouldNotifyNtfy = isPinHighEvent();
    shouldNotifyNtfy = shouldNotifyNtfy || (settings::ntfy::notify_heartbeat_tx && isHeartbeatTxEvent());
    shouldNotifyNtfy = shouldNotifyNtfy || isBootEvent();
    if (shouldNotifyNtfy)
        lmb_ntfy.sendMsg(jsonDoc);
}

void broadcastResults()
{
    serializeJson(jsonDoc, jsonString);
    lmb_wifi.sendMsg(jsonString);
    lmb_mqtt.sendMsg(jsonDoc);
    broadcastNtfy();
    if (settings::misc::serial_verbosity == 1)
    {
        uint16_t ctr = jsonDoc["RX"]["RX_COUNTER"] | 0;
        static uint16_t first_ctr = ctr;
        ctr -= first_ctr;

        unsigned long ms = millis();
        static unsigned long first_ms = ms;
        ms -= first_ms;

        Serial.printf("- ctr: %3u , ms: %8lu\n", ctr, ms);
    }
    else if (settings::misc::serial_verbosity == 2)
    {
        Serial.println(jsonString);
    }
}

void counterCheck()
{
    if (jsonDoc["TX"]["TX_COUNTER"].isNull())
        return;
    uint16_t txCnt = jsonDoc["TX"]["TX_COUNTER"].as<uint16_t>();
    static uint16_t prevTxCnt = txCnt - 1;
    static uint16_t errorCount = 0;
    uint16_t rxCnt = prevTxCnt + 1;
    bool counterStatus = txCnt != rxCnt;
    errorCount += counterStatus;

    jsonDoc["RX"]["RX_COUNTER"] = rxCnt;
    jsonDoc["RX_TX"]["RX_TX_COUNTER_ERROR_COUNT"] = errorCount;
    jsonDoc["RX_TX"]["RX_TX_COUNTER_STATUS"] = counterStatus ? "NOT OK" : "OK";

    prevTxCnt = txCnt;
}

String getCurrentTime()
{
    time_t now = time(nullptr);
    struct tm local_tm = *localtime(&now);
    struct tm utc_tm = *gmtime(&now);
    int offset_min = (local_tm.tm_hour - utc_tm.tm_hour) * 60 + (local_tm.tm_min - utc_tm.tm_min);
    if (local_tm.tm_mday != utc_tm.tm_mday)
        offset_min += (local_tm.tm_hour < 12) ? 1440 : -1440;
    int offset_h = offset_min / 60;
    int offset_m = abs(offset_min % 60);
    char timeStr[26];
    snprintf(timeStr, sizeof(timeStr), "%04d-%02d-%02dT%02d:%02d:%02d%+03d:%02d",
             local_tm.tm_year + 1900,
             local_tm.tm_mon + 1,
             local_tm.tm_mday,
             local_tm.tm_hour,
             local_tm.tm_min,
             local_tm.tm_sec,
             offset_h,
             offset_m);
    return String(timeStr);
}

void addLoraSettingsToJsonDoc()
{
    jsonDoc["LORA"]["LORA_FREQ"] = settings::lora::freq;
    jsonDoc["LORA"]["LORA_BW"] = settings::lora::bw;
    jsonDoc["LORA"]["LORA_SF"] = settings::lora::sf;
    jsonDoc["LORA"]["LORA_CR"] = settings::lora::cr;
    jsonDoc["LORA"]["LORA_SYNCWORD"] = settings::lora::syncword;
    jsonDoc["LORA"]["LORA_POWER"] = settings::lora::power;
    jsonDoc["LORA"]["LORA_PREAMBLE_LENGTH"] = settings::lora::preamble_length;
    jsonDoc["LORA"]["LORA_TCXO_VOLTAGE"] = settings::lora::tcxo_voltage;
    jsonDoc["LORA"]["LORA_USE_REGULATOR_LDO"] = settings::lora::use_regulator_ldo;
}

void heartBeat()
{
    unsigned long heartBeat = millis();
    static unsigned long prevHeartBeat = heartBeat;
    if (heartBeat - prevHeartBeat < settings::misc::rx_heartbeat_interval_ms)
        return;
    prevHeartBeat = heartBeat;

    jsonDoc.clear();
    jsonDoc["RX"]["RX_BOARD_ID"] = getMacAddress();
    jsonDoc["RX"]["RX_BUILD_LOCAL_TIME"] = BUILD_LOCAL_TIME;
    jsonDoc["RX"]["RX_GIT_HEAD_COMMIT_ID"] = GIT_HEAD_COMMIT_ID;
    jsonDoc["RX"]["RX_GIT_UNCOMMITTED_FILES_COUNT"] = GIT_UNCOMMITTED_FILES_COUNT;
    jsonDoc["RX"]["RX_TRIGGER"] = "HEARTBEAT_RX";
    jsonDoc["RX"]["RX_WEB_UI_URL"] = String("http://") + lmb_wifi.getLocalIP().toString();

    serializeJson(jsonDoc, jsonString);
    if (settings::misc::serial_verbosity == 2)
        Serial.println(jsonString);
    lmb_wifi.sendMsg(jsonString);
    lmb_mqtt.sendMsg(jsonDoc);
}

void readLoRa()
{
    digitalWrite(settings::board::lora_led_green, HIGH);
    radio.startReceive();
    int state = radio.readData(jsonString);
    digitalWrite(settings::board::lora_led_green, LOW);

    jsonDoc["RX_LORA_STATE"] = state;
    if (state != RADIOLIB_ERR_NONE)
        return;
    JsonDocument txDoc;
    DeserializationError deserializeError = deserializeJson(txDoc, jsonString);
    jsonDoc.clear();
    JsonObject tx = jsonDoc["TX"].to<JsonObject>();
    if (deserializeError)
    {
        tx["TX_JSON_DESERIALIZE_ERROR"] = deserializeError.c_str();
        tx["TX_JSON_STRING_HEX"] = bytesToHex(jsonString);
    }
    else
    {
        for (JsonPairConst kv : txDoc.as<JsonObjectConst>())
            tx[kv.key()] = kv.value();
        tx["TX_JSON_STRING"] = jsonString;
    }
    jsonDoc["RX"]["RX_BOARD_ID"] = getMacAddress();
    jsonDoc["RX"]["RX_BUILD_LOCAL_TIME"] = BUILD_LOCAL_TIME;
    jsonDoc["RX"]["RX_CURRENT_LOCAL_TIME"] = getCurrentTime();
    jsonDoc["RX"]["RX_DEBUG"] = settings::misc::debug;
    jsonDoc["RX"]["RX_GIT_HEAD_COMMIT_ID"] = GIT_HEAD_COMMIT_ID;
    jsonDoc["RX"]["RX_GIT_UNCOMMITTED_FILES_COUNT"] = GIT_UNCOMMITTED_FILES_COUNT;
    jsonDoc["RX"]["RX_TRIGGER"] = "LORA_PAYLOAD_RECEIVED";
    jsonDoc["RX"]["RX_WEB_UI_URL"] = String("http://") + lmb_wifi.getLocalIP().toString();
    jsonDoc["RX"]["RX_WS_CLIENT_COUNT"] = lmb_wifi.getWsClientCount();
    addLoraSettingsToJsonDoc();
    jsonDoc["RX_TX"]["RX_TX_RSSI_DBM"] = radio.getRSSI();
    jsonDoc["RX_TX"]["RX_TX_SNR_DB"] = radio.getSNR();
    if (!jsonDoc["TX"]["TX_VBAT_RAW"].isNull())
    {
        BatteryMeasurement battery = Vbat_raw2Vbat_mv(jsonDoc["TX"]["TX_VBAT_RAW"].as<uint16_t>());
        jsonDoc["TX"]["TX_VBAT_MV"] = battery.vbatMv;
        jsonDoc["TX"]["TX_VBAT_PERCENT"] = battery.batteryPercent;
        jsonDoc["TX"]["TX_VBAT_GLYPH"] = batteryGlyphToString(battery.glyph);
        jsonDoc["TX"]["TX_VBAT_STATUS"] = batteryStatusToString(battery.status);
    }
}

void setupMQTT()
{
    lmb_mqtt.begin();
    lmb_mqtt.sendMsg(jsonDoc);
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
     * @note On XIAO ESP32S3 with SX1262 shield, `settings::board::lora_user_button` and
     * `LED_BUILTIN` share GPIO21. Driving `LED_BUILTIN` high while pressing
     * the user button can create a short-circuit to GND.
     * @note Keep `pinMode(LED_BUILTIN, OUTPUT);` disabled for safety.
     */
    pinMode(settings::board::lora_led_green, OUTPUT);
}

void setup()
{
    setupGPIOs();
    blink(8UL, 200UL, 5UL, settings::board::lora_led_green, false);
    setupSerial();
    setupLoRa();
    setupLoRaRX();
    setupWiFi();
    setupMQTT();
}

void loop()
{
    lmb_wifi.ensureWiFiConnected();
    heartBeat();
    yield();
    if (!loraEvent)
        return;
    loraEvent = false;
    readLoRa();
    blink(8UL, 60UL, 5UL, settings::board::lora_led_green, false);
    counterCheck();
    broadcastResults();
    blink(8UL, 60UL, 5UL, settings::board::lora_led_green, false);
}
