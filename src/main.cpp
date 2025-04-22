#include <Arduino.h>

/*
    INPUT_PULLUP
    1 - contact ouvert - sans aimant (état normal)
    0 - contact fermé - avec aimant
*/

#define REED_1 GPIO_NUM_15
#define REED_2 GPIO_NUM_2
#define REED_3 GPIO_NUM_4

void readReed()
{
    int curVal = digitalRead(REED_3) * 100 + digitalRead(REED_2) * 10 + digitalRead(REED_1);
    static int lastVal = -1;
    if (lastVal == curVal)
        return;
    lastVal = curVal;

    digitalWrite(LED_BUILTIN, LOW);
    Serial.begin(BAUD_RATE);
    Serial.print("\nGPIO : ");
    Serial.printf("%03d", curVal);
    digitalWrite(LED_BUILTIN, HIGH);
}

void setup()
{
    pinMode(REED_1, INPUT);
    pinMode(REED_2, INPUT);
    pinMode(REED_3, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    uint64_t mask = (1ULL << REED_1) | (1ULL << REED_2) | (1ULL << REED_3);
    esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_HIGH);

    readReed();
    delay(1000); // debounce

    esp_deep_sleep_start();
}

void loop()
{
    yield();
    readReed();
}