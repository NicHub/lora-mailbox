#include <Arduino.h>

/*
    INPUT_PULLUP
    1 - contact ouvert - sans aimant (état normal)
    0 - contact fermé - avec aimant
*/

#define REED_1 GPIO_NUM_15
#define REED_2 GPIO_NUM_2
#define REED_3 GPIO_NUM_4

void readReed(bool forceRead)
{
    u_int8_t curVal = digitalRead(REED_3) * 100 + digitalRead(REED_2) * 10 + digitalRead(REED_1);
    static u_int8_t lastVal = curVal;
    if (!forceRead && lastVal == curVal)
        return;
    lastVal = curVal;

    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("\nGPIO : ");
    Serial.printf("%03d", curVal);
    Serial.print("  millis : ");
    Serial.print(millis());
    digitalWrite(LED_BUILTIN, HIGH);
}

void setupSerial()
{
    Serial.begin(BAUD_RATE);
}

void setupGPIOs()
{
    pinMode(REED_1, INPUT);
    pinMode(REED_2, INPUT);
    pinMode(REED_3, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
}

void setupDeepSleep()
{
    uint64_t mask = (1ULL << REED_1) | (1ULL << REED_2) | (1ULL << REED_3);
    esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_HIGH);
}

void debounce(uint32_t wait)
{
    delay(wait);
}

void goToDeepSleep()
{
    esp_deep_sleep_start();
}

void setup()
{
    setupGPIOs();
    setupSerial();
    setupDeepSleep();
    readReed(true);
    debounce(1000);
    goToDeepSleep();
}

void loop()
{
    yield();
    readReed(false);
}