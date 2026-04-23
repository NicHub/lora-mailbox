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

    ws.onEvent(
        [this](
            AsyncWebSocket *server,
            AsyncWebSocketClient *client,
            AwsEventType type,
            void *arg,
            uint8_t *data,
            size_t len)
        {
            if (type == WS_EVT_CONNECT)
            {
                Serial.printf("\nWebSocket client #%u connected\n", client->id());
                Serial.printf("Number of clients connected: %u\n", ws.count());
                if (latest_message.length() > 0)
                {
                    client->text(latest_message);
                }
            }
        });

    server.addHandler(&ws);

    server.on(
        "/",
        HTTP_GET,
        [](AsyncWebServerRequest *request) { request->send(200, "text/html", htmlTemplate); });

    server.begin();
    server_started = true;
}

const char *LoraMailboxWifi::encryptionToString(uint8_t type)
{
    switch (type)
    {
        case WIFI_AUTH_OPEN:             return "open";
        case WIFI_AUTH_WEP:              return "WEP";
        case WIFI_AUTH_WPA_PSK:          return "WPA";
        case WIFI_AUTH_WPA2_PSK:         return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:     return "WPA/WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE:  return "WPA2-Enterprise";
        case WIFI_AUTH_WPA3_PSK:         return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK:    return "WPA2/WPA3";
        case WIFI_AUTH_WAPI_PSK:         return "WAPI";
        default:                         return "unknown";
    }
}

void LoraMailboxWifi::scanAndCacheNetworks()
{
    last_scan.clear();

    Serial.println("Scanning for WiFi networks...");
    int networks_found = WiFi.scanNetworks();
    if (networks_found <= 0)
    {
        Serial.println(networks_found == 0 ? "No networks found!" : "WiFi scan failed");
        return;
    }
    last_scan.reserve(networks_found);
    for (int i = 0; i < networks_found; ++i)
    {
        ScanEntry entry;
        entry.ssid = WiFi.SSID(i);
        entry.bssid = WiFi.BSSIDstr(i);
        entry.rssi_dbm = WiFi.RSSI(i);
        entry.encryption_type = static_cast<uint8_t>(WiFi.encryptionType(i));
        last_scan.push_back(std::move(entry));
    }

    Serial.printf("%d networks found:\n", networks_found);
    const String connected_ssid = getConnectedSSID();
    const String connected_bssid = getConnectedBSSID();
    for (size_t i = 0; i < last_scan.size(); ++i)
    {
        const ScanEntry &e = last_scan[i];
        bool is_configured = false;
        for (const auto &cred : settings::wifi::NETWORKS)
        {
            if (cred.ssid != nullptr && cred.ssid[0] != '\0' && e.ssid == cred.ssid)
            {
                is_configured = true;
                break;
            }
        }
        bool is_connected = (e.ssid == connected_ssid && e.bssid == connected_bssid);
        Serial.printf(
            "  %u: %s (%d dBm) [%s]%s%s\n",
            static_cast<unsigned>(i + 1),
            e.ssid.c_str(),
            e.rssi_dbm,
            encryptionToString(e.encryption_type),
            is_configured ? " [configured]" : "",
            is_connected ? " [connected]" : "");
    }
}

void LoraMailboxWifi::registerNetworks()
{
    for (const auto &cred : settings::wifi::NETWORKS)
    {
        if (cred.ssid == nullptr || cred.ssid[0] == '\0')
            continue;
        wifi_multi.addAP(cred.ssid, cred.password);
        Serial.printf("Registered WiFi network: \"%s\"\n", cred.ssid);
    }
}

bool LoraMailboxWifi::begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);
    /**
     * @note setAutoReconnect(false): the ESP32 stack would otherwise silently re-attempt
     * the last used SSID on disconnect, which conflicts with WiFiMulti's scan-and-pick
     * strategy. We let WiFiMulti.run() drive every (re)connection.
     */
    WiFi.setAutoReconnect(false);

    registerNetworks();

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\nConnecting to WiFi (WiFiMulti)");
        uint8_t result = wifi_multi.run(settings::wifi::CONNECT_TIMEOUT_MS);
        if (result != WL_CONNECTED)
        {
            Serial.printf("\nWiFi connection failed (status=%u), retrying...\n", result);
            delay(settings::wifi::CONNECT_RETRY_DELAY_MS);
        }
    }

    Serial.printf("\nConnected to WiFi \"%s\" (%d dBm)\n", WiFi.SSID().c_str(), WiFi.RSSI());
    Serial.print("IP address: \nhttp://");
    Serial.println(WiFi.localIP());
    was_connected = true;

    scanAndCacheNetworks();
    synchronizeNTPTime();
    startServerIfNeeded();
    return true;
}

bool LoraMailboxWifi::ensureWiFiConnected()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        was_connected = true;
        return true;
    }

    uint32_t now = millis();
    if ((now - last_reconnect_attempt_ms) < settings::wifi::RECONNECT_MIN_INTERVAL_MS)
        return false;
    last_reconnect_attempt_ms = now;

    Serial.printf("WiFi disconnected (status=%d), trying WiFiMulti.run()...\n", WiFi.status());

    uint8_t result = wifi_multi.run(settings::wifi::RECONNECT_TIMEOUT_MS);
    if (result != WL_CONNECTED)
    {
        Serial.printf("WiFi reconnect failed (status=%u)\n", result);
        /** @note Extra delay after a full reconnection round failed. */
        if (was_connected)
        {
            last_reconnect_attempt_ms =
                now + settings::wifi::RECONNECT_RETRY_DELAY_MS
                - settings::wifi::RECONNECT_MIN_INTERVAL_MS;
            was_connected = false;
        }
        return false;
    }

    Serial.printf(
        "WiFi reconnected to \"%s\" (%d dBm), IP: ", WiFi.SSID().c_str(), WiFi.RSSI());
    Serial.println(WiFi.localIP());
    was_connected = true;
    just_reconnected = true;
    scanAndCacheNetworks();
    synchronizeNTPTime();
    return true;
}

bool LoraMailboxWifi::consumeReconnectedEvent()
{
    if (!just_reconnected)
        return false;
    just_reconnected = false;
    return true;
}

bool LoraMailboxWifi::synchronizeNTPTime()
{
    Serial.println("Synchronizing time with NTP server...");
    configTzTime(settings::ntp::TIMEZONE, settings::ntp::SERVER);

    /** @note Wait for time to be set. */
    time_t now = 0;
    struct tm timeinfo;
    int retry = 0;

    while (!getLocalTime(&timeinfo) && retry < settings::ntp::SYNC_MAX_RETRIES)
    {
        Serial.print(".");
        delay(settings::ntp::SYNC_RETRY_DELAY_MS);
        retry++;
    }

    if (retry < settings::ntp::SYNC_MAX_RETRIES)
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

String LoraMailboxWifi::getConnectedSSID() const
{
    return (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : String();
}

String LoraMailboxWifi::getConnectedBSSID() const
{
    return (WiFi.status() == WL_CONNECTED) ? WiFi.BSSIDstr() : String();
}

int32_t LoraMailboxWifi::getConnectedRSSI() const
{
    return (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
}
