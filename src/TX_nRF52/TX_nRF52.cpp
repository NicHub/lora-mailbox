/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

/*

Deep sleep from
https://forum.seeedstudio.com/t/xiao-sense-accelerometer-examples-and-low-power/270801

*/

// https://github.com/meshtastic/firmware/blob/master/variants/seeed_xiao_nrf52840_kit/variant.h
// Wio-SX1262 for XIAO (standalone SKU 113010003 or nRF52840 kit SKU 102010710)
// https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Wio-SX1262%20for%20XIAO%20V1.0_SCH.pdf
#define CS D4
#define IRQ D1
#define RST D2
#define LORA_GPIO_PIN D3
#define LORA_LED_GREEN LED_GREEN
#define BOARD_ID 1

#include <Arduino.h>
#include "common/common.h"
#include "common/common_nRF52.h"

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
