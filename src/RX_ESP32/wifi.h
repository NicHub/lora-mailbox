/**
 * LoRa MailBox — WebSocket
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "user_settings/user_settings.h"

class LoraMailboxWifi
{
private:
    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};
    String latest_message = "";
    uint32_t last_reconnect_attempt_ms = 0;
    bool server_started = false;

    bool waitForConnection(uint32_t timeout_ms);
    void startServerIfNeeded();

public:
    LoraMailboxWifi() {}

    void scanWiFiNetworks();
    bool begin();
    bool ensureWiFiConnected();
    bool synchronizeNTPTime();
    void sendMsg(const String &message);
    IPAddress getLocalIP();
    uint32_t getWsClientCount();
};
