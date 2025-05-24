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

void counterCheck(uint16_t cnt)
{
    static uint16_t lastCnt = cnt - 1;
    static uint16_t errorCount = 0;

#define PAD_LENGTH 20
    Serial.printf("  %-*s", PAD_LENGTH, "counter:");
    Serial.println(cnt);
    Serial.printf("  %-*s", PAD_LENGTH, "last counter:");
    Serial.println(lastCnt);
    Serial.printf("  %-*s", PAD_LENGTH, "counter status:");
    if (cnt == lastCnt + 1)
    {
        Serial.println("OK!");
    }
    else
    {
        ++errorCount;
        Serial.println("NOT OK!");
    }
    Serial.printf("  %-*s", PAD_LENGTH, "error count:");
    Serial.println(errorCount);
    lastCnt = cnt;
}

uint16_t readLoRa()
{
    String msg;
    int state = radio.readData(msg);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(PREFIX "No packet! state: %d", state);
        return 0;
    }
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);
    wifi.updateMessages(msg);

    uint16_t cnt = doc["cnt"];
    Serial.printf(PREFIX "Received:\t\t%s", msg.c_str());
    Serial.printf(PREFIX "cnt:\t\t%d", cnt);
    Serial.printf(PREFIX "RSSI:\t\t%.2f dBm", radio.getRSSI());
    Serial.printf(PREFIX "SNR:\t\t%.2f dB", radio.getSNR());
    return cnt;
}

void startReceive()
{
    Serial.print(F(PREFIX "Start receive"));
    radio.startReceive();
}

void setupWiFi()
{
    wifi.begin();
    wifi.updateMessages("Tchô");
}

void setup()
{
    setupSerial();
    setupLoRa();
    setupWiFi();

    // Set the function that will be called when new
    // packet is received.
    radio.setDio1Action(setFlag);
    radio.startReceive();

    // setupWifi();
    // setupMQTT();
}

void loop()
{
    yield();
    if (!loraEvent)
        return;
    loraEvent = false;
    startReceive();
    uint16_t cnt = readLoRa();
    counterCheck(cnt);

    // sendMail();
    // sendMQTT("Hello from ESP32!");
}
