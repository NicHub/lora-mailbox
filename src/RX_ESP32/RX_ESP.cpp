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
    JsonVariant trigger = jsonDoc["TX"]["TX_TRIGGER"];
    if (trigger.isNull())
        trigger = jsonDoc["TX"]["TX_WAKEUP"];
    if (trigger.isNull())
        trigger = jsonDoc["TX"]["WAKEUP"];
    if (trigger.isNull())
        trigger = jsonDoc["TX"]["wakeup"];
    if (trigger.isNull())
        return false;
    return String(trigger.as<const char *>()) == "WAKEUP_PIN_HIGH";
}

/**
 * @brief Return whether the current payload reports a TX heartbeat event.
 * @return true when the TX trigger equals `HEARTBEAT_TX`, false otherwise.
 */
bool isHeartbeatTxEvent()
{
    JsonVariant trigger = jsonDoc["TX"]["TX_TRIGGER"];
    if (trigger.isNull())
        trigger = jsonDoc["TX"]["TX_WAKEUP"];
    if (trigger.isNull())
        trigger = jsonDoc["TX"]["WAKEUP"];
    if (trigger.isNull())
        trigger = jsonDoc["TX"]["wakeup"];
    if (trigger.isNull())
        return false;
    return String(trigger.as<const char *>()) == "HEARTBEAT_TX";
}

/**
 * @brief Forward the current payload to NTFY when it matches notification rules.
 */
void broadcastNtfy()
{
    bool shouldNotifyNtfy = isPinHighEvent();
    shouldNotifyNtfy = shouldNotifyNtfy || (settings::ntfy::notify_heartbeat_tx && isHeartbeatTxEvent());
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
        uint16_t ctr = jsonDoc["COUNTER"]["VALUE"].as<uint16_t>();
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
    if (jsonDoc["TX"]["TX_CNT"].isNull())
        return;
    uint16_t cnt = jsonDoc["TX"]["TX_CNT"].as<uint16_t>();
    static uint16_t prevCnt = cnt - 1;
    static uint16_t errorCount = 0;
    bool counterStatus = cnt != prevCnt + 1;
    errorCount += counterStatus;

    jsonDoc["COUNTER"]["VALUE"] = cnt;
    jsonDoc["COUNTER"]["PREVIOUS_VALUE"] = prevCnt;
    jsonDoc["COUNTER"]["ERROR_COUNT"] = errorCount;
    jsonDoc["COUNTER"]["STATUS"] = counterStatus ? "NOT OK" : "OK";

    prevCnt = cnt;
}

String getCurrentTime()
{
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    char timeStr[25];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
    return String(timeStr);
}

void heartBeat()
{
    unsigned long heartBeat = millis();
    static unsigned long prevHeartBeat = heartBeat;
    if (heartBeat - prevHeartBeat < settings::lora::rx_heartbeat_interval_ms)
        return;
    prevHeartBeat = heartBeat;

    jsonDoc.clear();
    jsonDoc["RX"]["RX_BOARD_ID"] = getMacAddress();
    jsonDoc["RX"]["RX_COMPILATION_DATE"] = COMPILATION_DATE;
    jsonDoc["RX"]["RX_COMPILATION_TIME"] = COMPILATION_TIME;
    jsonDoc["RX"]["RX_HEARTBEAT"] = getCurrentTime();
    jsonDoc["RX"]["RX_LAST_COMMIT_ID"] = LAST_COMMIT_ID;
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
    jsonDoc["RX"]["RX_COMPILATION_DATE"] = COMPILATION_DATE;
    jsonDoc["RX"]["RX_COMPILATION_TIME"] = COMPILATION_TIME;
    jsonDoc["RX"]["RX_CURRENT_TIME"] = getCurrentTime();
    jsonDoc["RX"]["RX_LAST_COMMIT_ID"] = LAST_COMMIT_ID;
    jsonDoc["RX"]["RX_RSSI_DBM"] = radio.getRSSI();
    jsonDoc["RX"]["RX_SNR_DB"] = radio.getSNR();
    jsonDoc["RX"]["RX_WEB_UI_URL"] = String("http://") + lmb_wifi.getLocalIP().toString();
    jsonDoc["RX"]["RX_WS_CLIENT_COUNT"] = lmb_wifi.getWsClientCount();
    jsonDoc["RX_TX"]["RX_TX_DEBUG"] = settings::misc::debug;
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
