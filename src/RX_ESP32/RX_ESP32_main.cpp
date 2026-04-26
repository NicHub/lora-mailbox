/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include "RX_ESP32.h"

void setup()
{
    setupGPIOs();
    blink(8UL, 200UL, 5UL, settings::board::LORA_LED_GREEN, false);
    setupSerial();
    setupLoRa();
    setupLoRaRX();
    setupWiFi();
    setupMQTT();
}

void loop()
{
    statusLed.update();
    lmb_wifi.ensureWiFiConnected();
    if (lmb_wifi.consumeReconnectedEvent())
        publishRxWifiReconnectedMessage();
    heartBeat();
    yield();
    if (!loraEvent)
        return;
    loraEvent = false;
    readLoRa();
    counterCheck();
    broadcastResults();
    statusLed.start(8UL, 60UL, 10UL, settings::board::LORA_LED_GREEN, false);
}
