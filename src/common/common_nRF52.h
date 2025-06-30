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

// https://github.com/meshtastic/firmware/blob/master/variants/seeed_xiao_nrf52840_kit/variant.h
// Wio-SX1262 for XIAO (standalone SKU 113010003 or nRF52840 kit SKU 102010710)
// https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Wio-SX1262%20for%20XIAO%20V1.0_SCH.pdf
#define CS D4
#define IRQ D1
#define RST D2
#define LORA_GPIO_PIN D3
#define LORA_LED_GREEN LED_GREEN

using namespace Adafruit_LittleFS_Namespace;

#define NO_HEARTBEAT_PIN D0
#define LORA_USER_BUTTON GPIO_NUM_21

#define CNT_LOG_FILENAME "/cnt.log"

#define PREFIX "\n[" PROJECT_NAME "] "

File file(InternalFS);
Adafruit_LittleFS littleFS;

void debounce(uint32_t);

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

#if defined(FORMAT_LITTLEFS) && FORMAT_LITTLEFS == 1
    InternalFS.format();
    while (true)
        yield();
#endif

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

uint16_t readBatteryVoltage()
{
    // TODO
    return 0;
}