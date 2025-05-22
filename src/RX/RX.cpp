/**
 * LoRa MAILBOX
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include "common/common.h"
#include "common/LoraMailBox_Settings.h"
// #include "LoraMailBox_SendMail.h"
// #include "LoraMailBox_SendMQTT.h"

void readLora()
{
    String msg;
    int state = radio.readData(msg);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(PREFIX "No packet! state: %d", state);
        return;
    }
    static uint16_t cntMail = 0;
    cntMail++;
    blink();
    Serial.printf(PREFIX "cntMail:\t\t%d", cntMail);
    Serial.printf(PREFIX "Received:\t\t%s", msg.c_str());
    Serial.printf(PREFIX "RSSI:\t\t%.2f dBm", radio.getRSSI());
    Serial.printf(PREFIX "SNR:\t\t%.2f dB", radio.getSNR());
}

void startReceive()
{
    Serial.print(F(PREFIX "Start receive"));
    radio.startReceive();
}

void setupSerial()
{
    Serial.begin(BAUD_RATE);
    for (size_t i = 0; i < 3; i++)
    {
        Serial.println(i);
        delay(1000);
    }
}

void setup()
{
    setupSerial();
    setupLoRa();
    // setupWifi();
    // setupMQTT();
}

void loop()
{
    if (!loraEvent)
    {
        yield();
        return;
    }
    loraEvent = false;
    startReceive();
    readLora();
    // sendMail();
    // sendMQTT("Hello from ESP32!");
}
