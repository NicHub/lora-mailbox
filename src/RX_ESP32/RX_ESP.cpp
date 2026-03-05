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
#include "LoraMailBox_SendWS.h"
#include "LoraMailBox_SendMQTT.h"
#include "LoraMailBox_SendNTFY.h"

LoraMailBox_SendWS lmb_ws;
LoraMailBox_SendMQTT lmb_mqtt;
LoraMailBox_SendNTFY lmb_ntfy;
String jsonString;
JsonDocument jsonDoc;

/**
 * @brief Return whether the current payload reports a mailbox event.
 * @return true when `wakeup` equals `WAKEUP_PIN_HIGH`, false otherwise.
 */
bool isPinHighEvent()
{
    JsonVariant wakeup = jsonDoc["wakeup"];
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
    uint16_t batteryLevel = jsonDoc["volt_gpio"] | 0;
    return batteryLevel > 0 && batteryLevel <= MQTT_BATTERY_LOW_THRESHOLD_MV;
}

/**
 * @brief Return whether the current payload reports a TX heartbeat event.
 * @return true when `wakeup` equals `HEARTBEAT_TX`, false otherwise.
 */
bool isHeartbeatTxEvent()
{
    JsonVariant wakeup = jsonDoc["wakeup"];
    if (wakeup.isNull())
        return false;
    return String(wakeup.as<const char *>()) == "HEARTBEAT_TX";
}

/**
 * @brief Return whether the current payload should be forwarded to NTFY.
 * @return true on mailbox event, low battery event, or configured TX heartbeat event.
 */
bool shouldSendNtfy()
{
#if !NTFY_ENABLED
    return false;
#else
    bool heartbeatTx = false;
#if NTFY_NOTIFY_HEARTBEAT_TX
    heartbeatTx = isHeartbeatTxEvent();
#endif
    return isPinHighEvent() || isLowBatteryEvent() || heartbeatTx;
#endif
}

void broadcastResults()
{
    serializeJson(jsonDoc, jsonString);
    lmb_ws.sendMsg(jsonString);
    lmb_mqtt.sendMsg(jsonDoc);
    if (shouldSendNtfy())
        lmb_ntfy.sendMsg(jsonDoc);
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
    jsonDoc["COUNTER"]["PREVIOUS VALUE"] = prevCnt;
    jsonDoc["COUNTER"]["ERROR COUNT"] = errorCount;
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
    String timeStr = getCurrentTime();
    jsonString = "{\"HEARTBEAT_RX\":\"" + timeStr + "\"}";
    deserializeJson(jsonDoc, jsonString);
#if SERIAL_VERBOSITY == 2
    Serial.println(jsonString);
#endif
    lmb_ws.sendMsg(jsonString);
    lmb_mqtt.sendMsg(jsonDoc);
}

void readLoRa()
{
    digitalWrite(LORA_LED_GREEN, HIGH);
    radio.startReceive();
    int state = radio.readData(jsonString);
    digitalWrite(LORA_LED_GREEN, LOW);

    jsonDoc["LoRa STATE"] = state;
    if (state != RADIOLIB_ERR_NONE)
        return;
    deserializeJson(jsonDoc, jsonString);
    jsonDoc["CURRENT TIME"] = getCurrentTime();
    jsonDoc["COMPILATION DATE"] = COMPILATION_DATE;
    jsonDoc["COMPILATION TIME"] = COMPILATION_TIME;
    jsonDoc["RSSI (dBm)"] = radio.getRSSI();
    jsonDoc["SNR (dB)"] = radio.getSNR();
    jsonDoc["IP"] = lmb_ws.getLocalIP();
    jsonDoc["WS CLIENT COUNT"] = lmb_ws.getWsClientCount();
    jsonDoc["STATE"] = state;
    jsonDoc["jsonString"] = jsonString;
    jsonDoc["DEBUG"] = DEBUG;
}

void setupMQTT()
{
    lmb_mqtt.begin();
    lmb_mqtt.sendMsg(jsonDoc);
}

void setupWiFi()
{
    lmb_ws.begin();
    lmb_ws.synchronizeNTPTime();
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
    setupSerial();
    setupLoRa();
    setupLoRaRX();
    setupWiFi();
    setupMQTT();
}

void loop()
{
    if (digitalRead(NO_HEARTBEAT_PIN))
        heartBeat();
    yield();
    if (!loraEvent)
        return;
    loraEvent = false;
    readLoRa();
    counterCheck();
    broadcastResults();
}
