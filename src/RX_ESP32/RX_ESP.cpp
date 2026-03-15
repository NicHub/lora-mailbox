/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#define NO_HEARTBEAT_PIN D5
#define WAKEUP_PIN D10

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

/**
 * @brief Return whether the current payload reports a mailbox event.
 * @return true when `wakeup` equals `WAKEUP_PIN_HIGH`, false otherwise.
 */
bool isPinHighEvent()
{
    JsonVariant wakeup = jsonDoc["WAKEUP"];
    if (wakeup.isNull())
        wakeup = jsonDoc["wakeup"];
    if (wakeup.isNull())
        return false;
    return String(wakeup.as<const char *>()) == "WAKEUP_PIN_HIGH";
}

/**
 * @brief Return whether the current payload reports a low battery condition.
 * @return true when `volt_gpio` is below or equal to the configured threshold.
 */
bool isLowBatteryEvent()
{
    JsonVariant batteryStatus = jsonDoc["VBAT_STATUS"];
    if (batteryStatus.isNull())
        return false;
    return String(batteryStatus.as<const char *>()) == "LOW";
}

/**
 * @brief Return whether the current payload reports a TX heartbeat event.
 * @return true when `wakeup` equals `HEARTBEAT_TX`, false otherwise.
 */
bool isHeartbeatTxEvent()
{
    JsonVariant wakeup = jsonDoc["WAKEUP"];
    if (wakeup.isNull())
        wakeup = jsonDoc["wakeup"];
    if (wakeup.isNull())
        return false;
    return String(wakeup.as<const char *>()) == "HEARTBEAT_TX";
}

/**
 * @brief Forward the current payload to NTFY when it matches notification rules.
 */
void broadcastNtfy()
{
    bool shouldNotifyNtfy = isPinHighEvent() || isLowBatteryEvent();
#if NTFY_NOTIFY_HEARTBEAT_TX
    shouldNotifyNtfy = shouldNotifyNtfy || isHeartbeatTxEvent();
#endif
    if (shouldNotifyNtfy)
        lmb_ntfy.sendMsg(jsonDoc);
}

void broadcastResults()
{
    serializeJson(jsonDoc, jsonString);
    lmb_wifi.sendMsg(jsonString);
    lmb_mqtt.sendMsg(jsonDoc);
    broadcastNtfy();
#if SERIAL_VERBOSITY == 1
    uint16_t ctr = jsonDoc["COUNTER"]["VALUE"].as<uint16_t>();
    static uint16_t first_ctr = ctr;
    ctr -= first_ctr;

    unsigned long ms = millis();
    static unsigned long first_ms = ms;
    ms -= first_ms;

    Serial.printf("- ctr: %3u , ms: %8lu\n", ctr, ms);
#elif SERIAL_VERBOSITY == 2
    Serial.println(jsonString);
#endif
}

void counterCheck()
{
    uint16_t cnt = jsonDoc["cnt"].as<uint16_t>();
    static uint16_t prevCnt = cnt - 1;
    static uint16_t errorCount = 0;
    bool counterStatus = cnt != prevCnt + 1;
    errorCount += counterStatus;

    jsonDoc.remove("cnt");
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
    if (heartBeat - prevHeartBeat < 5000)
        return;
    prevHeartBeat = heartBeat;
    jsonDoc.clear();
    jsonDoc["HEARTBEAT_RX"] = getCurrentTime();
    jsonDoc["BOARD_ID_HEX"] = getMacAddress();
    jsonDoc["WEB_UI_URL"] = String("http://") + lmb_wifi.getLocalIP().toString();
    jsonDoc["COMPILATION_DATE"] = COMPILATION_DATE;
    jsonDoc["COMPILATION_TIME"] = COMPILATION_TIME;
    serializeJson(jsonDoc, jsonString);
#if SERIAL_VERBOSITY == 2
    Serial.println(jsonString);
#endif
    lmb_wifi.sendMsg(jsonString);
    lmb_mqtt.sendMsg(jsonDoc);
}

void readLoRa()
{
    digitalWrite(LORA_LED_GREEN, HIGH);
    radio.startReceive();
    int state = radio.readData(jsonString);
    digitalWrite(LORA_LED_GREEN, LOW);

    jsonDoc["LORA_STATE"] = state;
    if (state != RADIOLIB_ERR_NONE)
        return;
    deserializeJson(jsonDoc, jsonString);
    if (!jsonDoc["board_id_hex"].isNull())
    {
        jsonDoc["BOARD_ID_HEX"] = jsonDoc["board_id_hex"];
        jsonDoc.remove("board_id_hex");
    }
    if (!jsonDoc["wakeup"].isNull())
    {
        jsonDoc["WAKEUP"] = jsonDoc["wakeup"];
        jsonDoc.remove("wakeup");
    }
    if (!jsonDoc["volt_gpio"].isNull())
    {
        jsonDoc["VGPIO"] = jsonDoc["volt_gpio"];
        jsonDoc.remove("volt_gpio");
        BatteryMeasurement battery = Vgpio2Vbat(jsonDoc["VGPIO"].as<uint16_t>());
        jsonDoc["VBAT"] = battery.vbatMv;
        jsonDoc["VBAT_PERCENT"] = battery.batteryPercent;
        jsonDoc["VBAT_GLYPH"] = battery.glyph;
        jsonDoc["VBAT_STATUS"] = battery.status;
    }
    jsonDoc["CURRENT_TIME"] = getCurrentTime();
    jsonDoc["COMPILATION_DATE"] = COMPILATION_DATE;
    jsonDoc["COMPILATION_TIME"] = COMPILATION_TIME;
    jsonDoc["RSSI_DBM"] = radio.getRSSI();
    jsonDoc["SNR_DB"] = radio.getSNR();
    jsonDoc["WEB_UI_URL"] = String("http://") + lmb_wifi.getLocalIP().toString();
    jsonDoc["WS_CLIENT_COUNT"] = lmb_wifi.getWsClientCount();
    jsonDoc["STATE"] = state;
    jsonDoc["JSON_STRING"] = jsonString;
    jsonDoc["DEBUG"] = DEBUG;
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
     * @note On XIAO ESP32S3 with SX1262 shield, `LORA_USER_BUTTON` and
     * `LED_BUILTIN` share GPIO21. Driving `LED_BUILTIN` high while pressing
     * the user button can create a short-circuit to GND.
     * @note Keep `pinMode(LED_BUILTIN, OUTPUT);` disabled for safety.
     */
    pinMode(LORA_LED_GREEN, OUTPUT);
    pinMode(NO_HEARTBEAT_PIN, INPUT_PULLUP);
}

void setup()
{
    setupGPIOs();
    blink(8UL, 200UL, 5UL, LORA_LED_GREEN, false);
    setupSerial();
    setupLoRa();
    setupLoRaRX();
    setupWiFi();
    setupMQTT();
}

void loop()
{
    lmb_wifi.ensureWiFiConnected();
    if (digitalRead(NO_HEARTBEAT_PIN))
        heartBeat();
    yield();
    if (!loraEvent)
        return;
    loraEvent = false;
    readLoRa();
    blink(8UL, 60UL, 5UL, LORA_LED_GREEN, false);
    counterCheck();
    broadcastResults();
    blink(8UL, 60UL, 5UL, LORA_LED_GREEN, false);
}
