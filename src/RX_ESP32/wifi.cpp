/**
 * LoRa MailBox — WebSocket
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include "wifi.h"

#include "index.html"

bool LoraMailboxWifi::waitForConnection(uint32_t timeout_ms)
{
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms)
    {
        delay(250);
        Serial.print(".");
    }
    return WiFi.status() == WL_CONNECTED;
}

void LoraMailboxWifi::startServerIfNeeded()
{
    if (server_started)
        return;

    ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len)
               {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("\nWebSocket client #%u connected\n", client->id());
            Serial.printf("Number of clients connected: %u\n", ws.count());
            if (latest_message.length() > 0) {
                client->text(latest_message);
            }
        } });

    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", htmlTemplate); });

    server.begin();
    server_started = true;
}

void LoraMailboxWifi::scanWiFiNetworks()
{
    Serial.println("Scanning for WiFi networks...");
    int networks_found = WiFi.scanNetworks();
    if (networks_found == 0)
    {
        Serial.println("No networks found!");
        return;
    }
    Serial.print(networks_found);
    Serial.println(" networks found:");
    for (int i = 0; i < networks_found; ++i)
    {
        /** @note Print SSID and RSSI for each network found. */
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

bool LoraMailboxWifi::begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\nConnecting to WiFi");

        WiFi.begin(settings::wifi::ssid, settings::wifi::password);
        waitForConnection(settings::wifi::connect_timeout_ms);
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("\nWiFi connection failed");
            delay(settings::wifi::connect_retry_delay_ms);
            scanWiFiNetworks();
        }
    }

    Serial.println("\nConnected to WiFi");
    Serial.print("IP address: \nhttp://");
    Serial.println(WiFi.localIP());

    startServerIfNeeded();
    return true;
}

bool LoraMailboxWifi::ensureWiFiConnected()
{
    if (WiFi.status() == WL_CONNECTED)
        return true;

    uint32_t now = millis();
    if ((now - last_reconnect_attempt_ms) < settings::wifi::reconnect_min_interval_ms)
        return false;
    last_reconnect_attempt_ms = now;

    Serial.printf("WiFi disconnected (status=%d), trying reconnect...\n", WiFi.status());

    WiFi.disconnect(false, false);
    WiFi.begin(settings::wifi::ssid, settings::wifi::password);
    if (!waitForConnection(settings::wifi::reconnect_timeout_ms))
    {
        Serial.println("WiFi reconnect failed");
        return false;
    }

    Serial.print("WiFi reconnected, IP: ");
    Serial.println(WiFi.localIP());
    synchronizeNTPTime();
    return true;
}

bool LoraMailboxWifi::synchronizeNTPTime()
{
    Serial.println("Synchronizing time with NTP server...");
    configTzTime(settings::ntp::timezone, settings::ntp::server);

    /** @note Wait for time to be set. */
    time_t now = 0;
    struct tm timeinfo;
    int retry = 0;

    while (!getLocalTime(&timeinfo) && retry < settings::ntp::sync_max_retries)
    {
        Serial.print(".");
        delay(settings::ntp::sync_retry_delay_ms);
        retry++;
    }

    if (retry < settings::ntp::sync_max_retries)
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

void LoraMailboxWifi::sendMsg(const String &message)
{
    latest_message = message;
    ws.textAll(latest_message);
    ws.cleanupClients();
}

IPAddress LoraMailboxWifi::getLocalIP()
{
    return WiFi.localIP();
}

uint32_t LoraMailboxWifi::getWsClientCount()
{
    return ws.count();
}
