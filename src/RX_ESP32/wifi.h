/**
 * LoRa MailBox — WebSocket
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <vector>

#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "user_settings/user_settings.h"

class LoraMailboxWifi
{
  public:
    struct ScanEntry
    {
        String ssid;
        String bssid;
        int32_t rssi_dbm;
        uint8_t encryption_type;
    };

  private:
    AsyncWebServer server{80};
    AsyncWebSocket ws{"/ws"};
    WiFiMulti wifi_multi;
    String latest_message = "";
    std::vector<ScanEntry> last_scan;
    uint32_t next_reconnect_attempt_ms = 0;
    bool server_started = false;
    bool was_connected = false;
    bool just_reconnected = false;

    void startServerIfNeeded();
    void registerNetworks();
    bool connectBySignalStrength(uint32_t timeout_ms);
    bool connectInSettingsOrder(uint32_t timeout_ms);
    bool connectConfiguredNetwork(const char *ssid, const char *password, uint32_t timeout_ms);

  public:
    LoraMailboxWifi()
    {
    }

    void scanAndCacheNetworks();
    bool begin();
    bool ensureWiFiConnected();
    bool synchronizeNTPTime();
    void sendMsg(const String &message);
    IPAddress getLocalIP();
    uint32_t getWsClientCount();
    String getConnectedSSID() const;
    String getConnectedBSSID() const;
    int32_t getConnectedRSSI() const;
    bool isConfiguredSsid(const String &ssid) const;
    const std::vector<ScanEntry> &getLastScan() const { return last_scan; }
    /** @brief Returns true once after a successful reconnection; self-clears. */
    bool consumeReconnectedEvent();
    static const char *encryptionToString(uint8_t type);
};
