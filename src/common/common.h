/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <RadioLib.h>
#include <common/LoraMailBox_Settings.h>
#include <ArduinoJson.h>

#define PREFIX "\n[" PROJECT_NAME "] "
#define CNT_LOG_FILENAME "/cnt.log"

SX1262 radio = nullptr;

// Save transmission states between loops.
int transmissionState = RADIOLIB_ERR_NONE;

// Flag to indicate transmission or reception state.
bool transmitFlag = false;

// Flag to indicate that a packet was sent or received.
volatile bool loraEvent = false;

// The `setFlag` function is called when a complete
// packet is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type and
// MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void)
{
    loraEvent = true;
}

void debounce(uint32_t wait)
{
    delay(wait - millis() % 1000);
}

void blink(
    unsigned long on_duration_ms = 10,
    unsigned long total_duration_ms = 100,
    unsigned long repeat = 10)
{
    pinMode(LORA_LED_GREEN, OUTPUT);
    for (size_t i = 0; i < repeat; i++)
    {
        digitalWrite(LORA_LED_GREEN, HIGH);
        delay(on_duration_ms);
        digitalWrite(LORA_LED_GREEN, LOW);
        delay(total_duration_ms - on_duration_ms);
    }
}

void setupLoRa()
{
    radio = new Module(CS, IRQ, RST, GPIO);
    Serial.print(F(PREFIX "Initializing LoRa..."));
    int state = radio.begin(
        FREQ,
        BW,
        SF,
        CR,
        SYNCWORD,
        POWER,
        PREAMBLELENGTH,
        TCXOVOLTAGE,
        USEREGULATORLDO);
    if (state != RADIOLIB_ERR_NONE)
    {
        Serial.printf(" failed, code %d\n", state);
        while (true)
            yield();
    }
    Serial.print(F(" success"));
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

void setupSerial(size_t printCnt = 0)
{
    Serial.begin(BAUD_RATE);
    for (size_t i = 0; i <= printCnt; i++)
    {
        Serial.println(i);
        delay(1000);
    }
}

void switchOffAllLEDs()
{
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(LORA_LED_GREEN, LOW);
}