#include <Arduino.h>

/*
    INPUT_PULLUP
    1 - contact ouvert - sans aimant (état normal)
    0 - contact fermé - avec aimant
*/

void setup()
{
    pinMode(GPIO_NUM_0, INPUT);
    pinMode(GPIO_NUM_2, INPUT);
    pinMode(GPIO_NUM_4, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    uint64_t mask = (1ULL << GPIO_NUM_0) | (1ULL << GPIO_NUM_2) | (1ULL << GPIO_NUM_4);
    esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_HIGH);

    digitalWrite(LED_BUILTIN, LOW);
    Serial.begin(BAUD_RATE);
    Serial.print("GPIO : ");
    Serial.print(digitalRead(GPIO_NUM_0));
    Serial.print(digitalRead(GPIO_NUM_4));
    Serial.println(digitalRead(GPIO_NUM_2));
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000); // debounce

    esp_deep_sleep_start();
}

void loop()
{
    yield();
}
