/**
 * LoRa MailBox
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
#define BOARD_ID 0

#include <Arduino.h>
#include "common/common.h"
#include "common/common_ESP32.h"
#include "common/LoraMailBox_Settings.h"

void setup()
{
    setupGPIOs();
    setupSerial();
    setupLittleFS();
    setupLoRa();
}

void loop()
{
    uint16_t cnt = readMsgCounterFromFile();
    saveMsgCounterToFile(++cnt);
    uint16_t battery_voltage = readBatteryVoltage();
    bool stayAwake = digitalRead(STAY_AWAKE_PIN);
    Serial.print("\nstayAwake = ");
    Serial.println(stayAwake);
    transmitLoRa(BOARD_ID, cnt, battery_voltage, stayAwake);
    if (stayAwake)
        delay(3000);
    else
        goToDeepSleep();
}
