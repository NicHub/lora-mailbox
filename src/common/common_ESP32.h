/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include "common/LoraMailBox_Settings.h"

#define STAY_AWAKE_PIN D0
#define NO_HEARTBEAT_PIN D0
#define FORMAT_LITTLEFS_PIN D1
#define LORA_USER_BUTTON GPIO_NUM_21
#define WAKEUP_PIN GPIO_NUM_9
#define MASK (1ULL << WAKEUP_PIN)


void setupDeepSleep()
{
    esp_sleep_enable_ext1_wakeup(MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
}

void goToDeepSleep()
{
    Serial.printf(PREFIX "COMPILATION DATE %s", COMPILATION_DATE);
    Serial.printf(PREFIX "COMPILATION TIME %s", COMPILATION_TIME);
    Serial.printf(PREFIX "PROJECT NAME %s", PROJECT_NAME);
    Serial.print(F(PREFIX "GOING TO DEEP SLEEP\n"));
    Serial.flush();

    setupDeepSleep();
    debounce(1000);
    digitalWrite(LORA_LED_GREEN, LOW);
    esp_deep_sleep_start();
}

uint16_t readMsgCounterFromFile()
{
    File file = LittleFS.open(CNT_LOG_FILENAME, "r");
    uint16_t cnt = file.readString().toInt();
    file.close();
    return cnt;
}

void saveMsgCounterToFile(uint16_t cnt)
{
    File file = LittleFS.open(CNT_LOG_FILENAME, "w");
    file.print(cnt);
    file.close();
}

void setupLittleFS()
{
    bool state = LittleFS.begin(true);
    while (!state)
    {
        Serial.println("LittleFS.begin() failed");
        delay(1000);
    }

    if (digitalRead(FORMAT_LITTLEFS_PIN))
    {
        digitalWrite(LORA_LED_GREEN, HIGH);
        LittleFS.format();
        digitalWrite(LORA_LED_GREEN, LOW);
        while (digitalRead(FORMAT_LITTLEFS_PIN))
            yield();
    }

    if (!LittleFS.exists(CNT_LOG_FILENAME))
        saveMsgCounterToFile(0);
}

void switchOffAllLEDs()
{
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LORA_LED_GREEN, LOW);
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
    pinMode(LORA_USER_BUTTON, INPUT);
    pinMode(LORA_LED_GREEN, OUTPUT);
    pinMode(NO_HEARTBEAT_PIN, INPUT);
}

uint16_t readBatteryVoltage()
{
    // TODO
    return 0;
}