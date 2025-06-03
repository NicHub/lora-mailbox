/**
 * LoRa MailBox — WebSocket
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "credentials.h"

class LoraMailBox_SendWS
{
private:
    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};
    String latestMessage = "";

#include "index.html"

public:
    LoraMailBox_SendWS() {}

    void scanWiFiNetworks()
    {
        Serial.println("Scanning for WiFi networks...");
        int networksFound = WiFi.scanNetworks();
        if (networksFound == 0)
        {
            Serial.println("No networks found!");
            return;
        }
        Serial.print(networksFound);
        Serial.println(" networks found:");
        for (int i = 0; i < networksFound; ++i)
        {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(" dBm)");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [open]" : " [encrypted]");
            delay(10);
        }
    }

    bool begin()
    {
        while (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("\nConnecting to WiFi");

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
                Serial.println("\nWiFi connection failed");
                delay(500);
                scanWiFiNetworks();
            }

        }

        Serial.println("\nConnected to WiFi");
        Serial.print("IP address: \nhttp://");
        Serial.println(WiFi.localIP());

        // Set up WebSocket handler.
        ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                          AwsEventType type, void *arg, uint8_t *data, size_t len)
                   {
            if (type == WS_EVT_CONNECT) {
                Serial.printf("\nWebSocket client #%u connected\n", client->id());
                Serial.printf("Number of clients connected: %u\n", ws.count());
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

    bool synchronizeNTPTime()
    {
        const char *ntpServer = "pool.ntp.org";
        const long gmtOffset_sec = 3600;     // Adjust for your timezone (e.g., 3600 for GMT+1)
        const int daylightOffset_sec = 3600; // Daylight saving time offset (if applicable)

        Serial.println("Synchronizing time with NTP server...");
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

        // Wait for time to be set
        time_t now = 0;
        struct tm timeinfo;
        int retry = 0;
        const int maxRetries = 10;

        while (!getLocalTime(&timeinfo) && retry < maxRetries)
        {
            Serial.print(".");
            delay(500);
            retry++;
        }

        if (retry < maxRetries)
        {
            time(&now);
            Serial.println("\nTime synchronized successfully");
            Serial.print("Current time: ");
            Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
            return true;
        }
        else
        {
            Serial.println("\nFailed to synchronize time");
            return false;
        }
    }

    void sendMsg(const String &message)
    {
        latestMessage = message;
        ws.textAll(latestMessage);
        ws.cleanupClients();
    }

    IPAddress getLocalIP()
    {
        return WiFi.localIP();
    }

    uint32_t getWsClientCount()
    {
        return ws.count();
    }
};
