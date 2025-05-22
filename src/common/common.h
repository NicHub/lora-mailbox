/**
 * LoRa MAILBOX
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <Arduino.h>
#include <LittleFS.h>
#include <RadioLib.h>
#include <common/LoraMailBox_Settings.h>

#define PREFIX "\n[" PROJECT_NAME "] "
#define CNT_LOG_FILENAME "/cnt.log"

SX1262 radio = nullptr; // SX1262

// save transmission states between loops
int transmissionState = RADIOLIB_ERR_NONE;

// flag to indicate transmission or reception state
bool transmitFlag = false;

// flag to indicate that a packet was sent or received
volatile bool loraEvent = false;

// this function is called when a complete packet
// is transmitted or received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void)
{
    loraEvent = true;
}

void blink(
    unsigned long on_duration_ms = 10,
    unsigned long total_duration_ms = 100,
    unsigned long repeat = 10)
{
    pinMode(LORA_GREEN_LED, OUTPUT);
    for (size_t i = 0; i < repeat; i++)
    {
        digitalWrite(LORA_GREEN_LED, HIGH);
        delay(on_duration_ms);
        digitalWrite(LORA_GREEN_LED, LOW);
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

uint16_t getMsgCounterFromFile()
{
    uint16_t cnt = 0;
    File fichier = LittleFS.open(CNT_LOG_FILENAME, "r");
    if (!fichier)
        return 0;
    String contenu = fichier.readString();
    fichier.close();
    cnt = contenu.toInt();
    return cnt;
}

void saveMsgCounterToFile(uint16_t cnt)
{
    File fichier = LittleFS.open(CNT_LOG_FILENAME, "w");
    if (!fichier)
        return;
    fichier.print(cnt);
    fichier.close();
    Serial.printf(PREFIX "New value saved: %d", cnt);
}

void setupLittleFS()
{
    while (!LittleFS.begin(true))
        delay(100);

    if (!LittleFS.exists(CNT_LOG_FILENAME))
        saveMsgCounterToFile(0);
}
