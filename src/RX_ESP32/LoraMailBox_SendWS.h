/**
 * LoRa MailBox — WebSocket
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "../common/user_settings.h"

class LoraMailBox_SendWS
{
private:
    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};
    String latestMessage = "";
    uint32_t lastReconnectAttemptMs = 0;
    bool serverStarted = false;

#include "index.html"

    /**
     * @brief Wait for Wi-Fi connection for a bounded amount of time.
     * @param timeoutMs Timeout in milliseconds.
     * @return true when connected before timeout.
     */
    bool waitForConnection(uint32_t timeoutMs)
    {
        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs)
        {
            delay(250);
            Serial.print(".");
        }
        return WiFi.status() == WL_CONNECTED;
    }

    /**
     * @brief Configure and start the local web server once.
     */
    void startServerIfNeeded()
    {
        if (serverStarted)
            return;

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

        server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request)
                  { request->send(200, "text/html", htmlTemplate); });

        server.begin();
        serverStarted = true;
    }

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
        WiFi.mode(WIFI_STA);
        WiFi.persistent(false);
        WiFi.setAutoReconnect(true);

        while (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("\nConnecting to WiFi");

            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
            waitForConnection(10000);
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

        startServerIfNeeded();
        return true;
    }

    /**
     * @brief Try to restore Wi-Fi connection when disconnected.
     * @return true when Wi-Fi is connected.
     */
    bool ensureWiFiConnected()
    {
        if (WiFi.status() == WL_CONNECTED)
            return true;

        uint32_t now = millis();
        if ((now - lastReconnectAttemptMs) < 5000)
            return false;
        lastReconnectAttemptMs = now;

        Serial.printf("WiFi disconnected (status=%d), trying reconnect...\n", WiFi.status());

        WiFi.disconnect(false, false);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        if (!waitForConnection(8000))
        {
            Serial.println("WiFi reconnect failed");
            return false;
        }

        Serial.print("WiFi reconnected, IP: ");
        Serial.println(WiFi.localIP());
        synchronizeNTPTime();
        return true;
    }

    bool synchronizeNTPTime()
    {
        const char *ntpServer = "pool.ntp.org";

        Serial.println("Synchronizing time with NTP server...");
        configTzTime(NTP_TIMEZONE, ntpServer);

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
            Serial.printf("DST active: %s\n", timeinfo.tm_isdst > 0 ? "yes" : "no");
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
