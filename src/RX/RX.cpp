/**
 * LoRa MailBox — RX
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include "common/common.h"
#include "common/LoraMailBox_Settings.h"
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

void getCurrentTime()
{
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    char timeStr[25];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
    jsonDoc["CURRENT TIME"] = timeStr;
}

void heartBeat()
{
    unsigned long heartBeat = millis();
    static unsigned long prevHeartBeat = heartBeat;
    if (heartBeat - prevHeartBeat < 5000)
        return;
    jsonString = "{\"HEARTBEAT\":" + String(heartBeat) + "}";
    deserializeJson(jsonDoc, jsonString);
    Serial.println(jsonString);
    lmb_ws.sendMsg(jsonString);
    lmb_mqtt.sendMsg(jsonDoc);
    prevHeartBeat = heartBeat;
}

void readLoRa()
{
    radio.startReceive();
    int state = radio.readData(jsonString);
    deserializeJson(jsonDoc, jsonString);
    getCurrentTime();
    jsonDoc["COMPILATION DATE"] = COMPILATION_DATE;
    jsonDoc["COMPILATION TIME"] = COMPILATION_TIME;
    jsonDoc["LoRa STATE"] = state;
    if (state != RADIOLIB_ERR_NONE)
        return;
    jsonDoc["RSSI (dBm)"] = radio.getRSSI();
    jsonDoc["SNR (dB)"] = radio.getSNR();
    jsonDoc["IP"] = lmb_ws.getLocalIP();
    jsonDoc["WS CLIENT COUNT"] = lmb_ws.getWsClientCount();
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

void setup()
{
    setupSerial();
    setupLoRa();
    setupLoRaRX();
    setupWiFi();
    setupMQTT();
    delay(1000 - millis() % 1000);
}

void loop()
{
    heartBeat();
    yield();
    if (!loraEvent)
        return;
    loraEvent = false;
    readLoRa();
    counterCheck();
    broadcastResults();
    blink();
}
