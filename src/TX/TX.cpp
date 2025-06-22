/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include "common/common.h"
#include "common/LoraMailBox_Settings.h"

void setupDeepSleep()
{
    esp_sleep_enable_ext1_wakeup(MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
}

void goToDeepSleep()
{
    setupDeepSleep();
    Serial.printf(PREFIX "Compilation date %s", COMPILATION_DATE);
    Serial.printf(PREFIX "Compilation time %s", COMPILATION_TIME);
    Serial.print(F(PREFIX "Going to deep sleep\n"));
    Serial.flush();
    esp_deep_sleep_start();
}

void stayAwake()
{
    yield();
    blink();
}

void debounce(uint32_t wait)
{
    delay(wait - millis() % 1000);
}

void transmitLoRa(uint16_t cnt)
{
    String msg;
    JsonDocument doc;
    doc["cnt"] = cnt;
    serializeJson(doc, msg);

    Serial.printf(PREFIX "Sending\t\t%s", msg.c_str());

    // Don’t use the non-blocking `startTransmit()` function.
    // It makes it difficult to estimate the necessary delay
    // before sending a new message.
    int state = radio.transmit(msg.c_str());

    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(PREFIX "Failed, code %d", state);
        return;
    }
    Serial.print(F(PREFIX "Transmission finished!"));
}

void transmitLoRa2(uint16_t cnt)
{
    String msg;
    JsonDocument doc;
    doc["cnt"] = cnt;
    serializeJson(doc, msg);

    Serial.printf(PREFIX "Sending\t\t%s", msg.c_str());
    int state = radio.transmit(msg.c_str());
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(PREFIX "Failed, code %d", state);
        return;
    }
    Serial.print(F(PREFIX "Transmission finished!"));
}

void setupGPIOs()
{
    // Beware that if you use XIAO ESP32S3 with LoRa
    // shield SX1262, LORA_USER_BUTTON on LoRa shield
    // and BUILTIN_LED on ESP module are both connected
    // on GPIO21! So if you set BUILTIN_LED pinMode to
    // OUTPUT and you set BUILTIN_LED to HIGH and you
    // press LORA_USER_BUTTON, you create a
    // short-circuit between GPIIO21 and GND.
    pinMode(LORA_USER_BUTTON, INPUT);
    pinMode(LORA_GREEN_LED, OUTPUT);

    pinMode(PIR_PIN_0, INPUT);
    pinMode(STAY_AWAKE_PIN, INPUT_PULLDOWN);
}

void setup()
{
    setupGPIOs();
    setupSerial();
    setupLittleFS();
    setupLoRa();
}

void loop()
{
    digitalWrite(LORA_GREEN_LED, HIGH);
    uint16_t cnt = readMsgCounterFromFile();
    transmitLoRa(cnt);
    saveMsgCounterToFile(++cnt);
    digitalWrite(LORA_GREEN_LED, LOW);
    debounce(1000);
    if (digitalRead(STAY_AWAKE_PIN))
        stayAwake();
    else
        goToDeepSleep();
}
