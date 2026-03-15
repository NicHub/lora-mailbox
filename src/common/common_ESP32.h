/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define MASK (1ULL << WAKEUP_PIN)

// https://github.com/radiolib-org/RadioBoards/blob/main/src/maintained/SeeedStudio/XIAO_ESP32S3.h
// https://github.com/Seeed-Studio/one_channel_hub/blob/4cc771ac02da1bd18be67509f6b52d21bb0feabd/components/smtc_ral/bsp/sx126x/seeed_xiao_esp32s3_devkit_sx1262.c#L358-L369
#define CS 41            /* GPIO4 */
#define IRQ 39           /* DIO1 */
#define RST 42           /* GPIO3 */
#define LORA_GPIO_PIN 40 /* GPIO2 */

#define LORA_LED_GREEN GPIO_NUM_48
#define LORA_USER_BUTTON GPIO_NUM_21

#define PREFIX "\n[" PROJECT_NAME "] "

void debounce(uint32_t);
void blink(unsigned long, unsigned long, unsigned long, uint32_t, bool);

// Platform counter storage API.
// ESP32 RX does not currently persist a message counter, but we keep the
// same interface as nRF52 so a future ESP-based TX can reuse the same calls.
void setupMsgCounterStorage()
{
}

uint16_t readMsgCounter()
{
    return 0;
}

void saveMsgCounter(uint16_t cnt)
{
    (void)cnt;
}

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

void switchOffAllLEDs()
{
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LORA_LED_GREEN, LOW);
}

uint16_t readBatteryVoltage()
{
    // TODO
    return 0;
}

/**
 * @brief Return ESP32 station MAC address as `XX:XX:XX:XX:XX:XX`.
 * @return MAC address string.
 */
static inline String getMacAddress()
{
    return WiFi.macAddress();
}
