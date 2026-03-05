/**
 * LoRa MailBox — NTFY
 *
 * https://ntfy.sh/
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "../common/user_settings.h"

class LoraMailBox_SendNTFY
{
private:
    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};
    String latestMessage = "";

#include "index.html"

public:
    LoraMailBox_SendNTFY() {}

    // Send a plaintext notification to ntfy.sh via HTTPS.
    // Returns true on HTTP 2xx success, false otherwise.
    bool sendMsg(const JsonDocument &jsonDoc, const String &topic = NTFY_TOPIC)
    {
#if NTFY_ENABLED
        if (WiFi.status() != WL_CONNECTED)
            return false;

        WiFiClientSecure client;
        // For simplicity in this project we skip certificate validation.
        // In production, prefer setting a CA cert with client.setCACert(...).
        client.setInsecure();

        HTTPClient https;
        String url = String(NTFY_SERVER) + topic;

        bool ok = false;
        if (!https.begin(client, url))
            return false;

        String message;
        serializeJson(jsonDoc, message);

        if (String(NTFY_USERNAME).length() > 0 || String(NTFY_PASSWORD).length() > 0)
            https.setAuthorization(NTFY_USERNAME, NTFY_PASSWORD);

        https.addHeader("Content-Type", "application/json");
        int httpCode = https.POST(message);
        // consider 2xx codes as success
        ok = (httpCode >= 200 && httpCode < 300);
        https.end();

        return ok;
#else
        return true;
#endif
    }
};
