/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include "common/common.h"
#include "common/LoraMailBox_Settings.h"
#include "LoraMailBox_WiFi.h"

LoraMailBox_WiFi wifi;
String jsonString;
JsonDocument jsonDoc;

void counterCheck()
{
    uint16_t cnt = jsonDoc["cnt"].as<uint16_t>();
    static uint16_t lastCnt = cnt - 1;
    static uint16_t errorCount = 0;
    bool counterStatus = cnt != lastCnt + 1;
    errorCount += counterStatus;

    jsonDoc.remove("cnt");
    jsonDoc["COUNTER"]["VALUE"] = cnt;
    jsonDoc["COUNTER"]["LAST VALUE"] = lastCnt;
    jsonDoc["COUNTER"]["ERROR COUNT"] = errorCount;
    jsonDoc["COUNTER"]["STATUS"] = counterStatus ? "NOT OK" : "OK";

    lastCnt = cnt;
}

void readLoRa()
{
    radio.startReceive();
    int state = radio.readData(jsonString);
    deserializeJson(jsonDoc, jsonString);
    jsonDoc["COMPILATION DATE"] = COMPILATION_DATE;
    jsonDoc["COMPILATION TIME"] = COMPILATION_TIME;
    jsonDoc["LoRa STATE"] = state;
    if (state != RADIOLIB_ERR_NONE)
        return;
    jsonDoc["RSSI (dBm)"] = radio.getRSSI();
    jsonDoc["SNR (dB)"] = radio.getSNR();
    jsonDoc["IP"] = wifi.getLocalIP();
}

void setupWiFi()
{
    wifi.begin();
    wifi.sendWsMsg(PROJECT_NAME);
}

void setupLoRaRX()
{
    radio.setDio1Action(setFlag);
    radio.startReceive();
}

void broadcastResults()
{
    serializeJsonPretty(jsonDoc, jsonString);
    wifi.sendWsMsg(jsonString);
    Serial.println(jsonString);
}

void setup()
{
    setupSerial();
    setupLoRa();
    setupLoRaRX();
    setupWiFi();
}

void loop()
{
    yield();
    if (!loraEvent)
        return;
    loraEvent = false;
    readLoRa();
    counterCheck();
    broadcastResults();
}
