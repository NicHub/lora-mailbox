/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "credentials.h"

class LoraMailBox_WiFi
{
private:
    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};
    String latestMessage = "";

#include "index.html"

public:
    LoraMailBox_WiFi() {}

    bool begin()
    {
        // Connect to WiFi.
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // Wait for connection.
        uint8_t attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20)
        {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("\nWiFi connection failed!");
            return false;
        }

        Serial.println("\nConnected to WiFi");
        Serial.print("IP address: \nhttp://");
        Serial.println(WiFi.localIP());

        // Set up WebSocket handler.
        ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                          AwsEventType type, void *arg, uint8_t *data, size_t len)
                   {
            if (type == WS_EVT_CONNECT) {
                Serial.printf("WebSocket client #%u connected\n", client->id());
                if (latestMessage.length() > 0) {
                    client->text(latestMessage);
                }
            } });

        server.addHandler(&ws);

        // Route for root / web page.
        server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
                  { request->send(200, "text/html", htmlTemplate); });

        // Start server.
        server.begin();
        return true;
    }

    void sendWsMsg(const String &message)
    {
        latestMessage = message;
        ws.textAll(latestMessage);
        // ws.cleanupClients();
    }

    IPAddress getLocalIP()
    {
        return WiFi.localIP();
    }
};
