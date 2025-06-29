/**
 * LoRa MailBox — RX
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

LoraMailBox_SendWS lmb_ws;
LoraMailBox_SendMQTT lmb_mqtt;
String jsonString;
JsonDocument jsonDoc;

void broadcastResults()
{
    serializeJsonPretty(jsonDoc, jsonString);
    lmb_ws.sendMsg(jsonString);
    lmb_mqtt.sendMsg(jsonDoc);
    Serial.println(jsonString);
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
    jsonString = "{\"HEARTBEAT\":\"" + timeStr + "\"}";
    deserializeJson(jsonDoc, jsonString);
    Serial.println(jsonString);
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
}

void setupMQTT()
{
    lmb_mqtt.begin();
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

void setupGPIOs()
{
    // Beware that if you use XIAO ESP32S3 with LoRa
    // shield SX1262, LORA_USER_BUTTON on LoRa shield
    // and LED_BUILTIN on ESP module are both connected
    // on GPIO21! So if you set LED_BUILTIN pinMode to
    // OUTPUT and you set LED_BUILTIN to HIGH and you
    // press LORA_USER_BUTTON, you create a
    // short-circuit between GPIIO21 and GND.

    // pinMode(LED_BUILTIN, OUTPUT); => Too dangerous to use!
    pinMode(NO_HEARTBEAT_PIN, INPUT);
}

void setup()
{
    setupGPIOs();
    setupSerial();
    setupLoRa();
    setupLoRaRX();
    setupWiFi();
    setupMQTT();
    delay(1000 - millis() % 1000);
}

void loop()
{
    if(digitalRead(NO_HEARTBEAT_PIN))
        heartBeat();
    yield();
    if (!loraEvent)
        return;
    loraEvent = false;
    readLoRa();
    counterCheck();
    broadcastResults();
}
