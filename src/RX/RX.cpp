/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include "common/common.h"
#include "common/LoraMailBox_Settings.h"
#include "LoraMailBox_WiFi.h"

#define PAD_LENGTH 20

LoraMailBox_WiFi wifi;
String jsonString;
JsonDocument jsonDoc;

void counterCheck()
{
    static uint16_t lastCnt = (uint16_t)jsonDoc["cnt"] - 1;
    static uint16_t errorCount = 0;

    if ((uint16_t)jsonDoc["cnt"] != lastCnt + 1)
    {
        ++errorCount;
        jsonDoc["counter status"] = "NOT OK";
    }
    else
        jsonDoc["counter status"] = "OK";

    jsonDoc["last counter"] = lastCnt;
    jsonDoc["error count"] = errorCount;
    lastCnt = jsonDoc["cnt"];
}

void readLoRa()
{
    int state = radio.readData(jsonString);
    deserializeJson(jsonDoc, jsonString);
    jsonDoc["LoRa state"] = state;
    if (state != RADIOLIB_ERR_NONE)
        return;
    jsonDoc["RSSI (dBm)"] = radio.getRSSI();
    jsonDoc["SNR (dB)"] = radio.getSNR();
    jsonDoc["IP"] = wifi.getLocalIP();
}

void startReceive()
{
    radio.startReceive();
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
    startReceive();
    readLoRa();
    counterCheck();

    serializeJsonPretty(jsonDoc, jsonString);
    wifi.sendWsMsg(jsonString);
    Serial.println(jsonString);
}
