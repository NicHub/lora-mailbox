/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include "ESP32.h"
#include "common/common.h"


/**
 * @note Platform counter storage API.
 * @note ESP32 RX does not currently persist a message counter, but it keeps the
 * same interface as nRF52 so a future ESP-based TX can reuse the same calls.
 */
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
    Serial.printf("%sBUILD LOCAL TIME %s", PREFIX, BUILD_LOCAL_TIME);
    Serial.printf("%sPROJECT NAME %s", PREFIX, PROJECT_NAME);
    Serial.print(PREFIX);
    Serial.print(F("GOING TO DEEP SLEEP\n"));
    Serial.flush();

    setupDeepSleep();
    debounce(1000);
    digitalWrite(settings::board::LORA_LED_GREEN, LOW);
    esp_deep_sleep_start();
}

void switchOffAllLEDs()
{
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(settings::board::LORA_LED_GREEN, LOW);
}

uint16_t readBatteryVoltage()
{
    /** @todo Implement battery voltage reading on ESP32 builds. */
    return 0;
}
