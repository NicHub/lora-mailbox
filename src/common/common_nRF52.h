/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include "common/LoraMailBox_Settings.h"

using namespace Adafruit_LittleFS_Namespace;

#define STAY_AWAKE_PIN D6
#define NO_HEARTBEAT_PIN D0
#define FORMAT_LITTLEFS_PIN D1
#define LORA_USER_BUTTON GPIO_NUM_21
#define WAKEUP_PIN D10
#define MASK (1ULL << WAKEUP_PIN)

File file(InternalFS);
Adafruit_LittleFS littleFS;

void goToDeepSleep()
{
    debounce(1000);
    NRF_POWER->SYSTEMOFF = 1;
}

uint16_t readMsgCounterFromFile()
{
    file = InternalFS.open(CNT_LOG_FILENAME, FILE_O_READ);
    uint16_t cnt = file.readString().toInt();
    file.close();
    return cnt;
}

void saveMsgCounterToFile(uint16_t cnt)
{
    InternalFS.remove(CNT_LOG_FILENAME);
    file = InternalFS.open(CNT_LOG_FILENAME, FILE_O_WRITE);
    file.print(cnt, 10);
    file.close();
}

void setupLittleFS()
{
    bool state = InternalFS.begin();
    while (!state)
    {
        Serial.println("InternalFS.begin() failed");
        delay(1000);
    }

    if (digitalRead(FORMAT_LITTLEFS_PIN))
    {
        InternalFS.format();
        while (digitalRead(FORMAT_LITTLEFS_PIN))
            yield();
    }

    if (!InternalFS.exists(CNT_LOG_FILENAME))
        saveMsgCounterToFile(0);
}

void switchOffAllLEDs()
{
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH);
}

void setupGPIOs()
{
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    switchOffAllLEDs();

    pinMode(WAKEUP_PIN, INPUT_SENSE_HIGH);

    pinMode(STAY_AWAKE_PIN, INPUT);
    pinMode(FORMAT_LITTLEFS_PIN, INPUT);
}

uint16_t readBatteryVoltage()
{
    // TODO
    return 0;
}