/**
 * LoRa MailBox — RX
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

// https://github.com/radiolib-org/RadioBoards/blob/main/src/maintained/SeeedStudio/XIAO_ESP32S3.h
// https://github.com/Seeed-Studio/one_channel_hub/blob/4cc771ac02da1bd18be67509f6b52d21bb0feabd/components/smtc_ral/bsp/sx126x/seeed_xiao_esp32s3_devkit_sx1262.c#L358-L369
#define CS 41
#define IRQ 39
#define RST 42
#define LORA_GPIO_PIN 40
#define LORA_LED_GREEN GPIO_NUM_48

#include <Arduino.h>
#include "common/common.h"
#include "common/common_ESP32.h"
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
