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

    /**
     * @brief Build notification emoji text from message categories.
     * @param jsonDoc JSON payload.
     * @return Notification text.
     */
    String getNotificationText(const JsonDocument &jsonDoc) const
    {
        const char *wakeup = jsonDoc["wakeup"] | "";
        uint16_t batteryLevel = jsonDoc["volt_gpio"] | 0;
        bool pinHigh = strcmp(wakeup, "WAKEUP_PIN_HIGH") == 0;
        bool lowBattery = batteryLevel > 0 && batteryLevel <= MQTT_BATTERY_LOW_THRESHOLD_MV;
        bool heartbeatTx = false;
#if NTFY_NOTIFY_HEARTBEAT_TX
        heartbeatTx = strcmp(wakeup, "HEARTBEAT_TX") == 0;
#endif

        String text;
        if (pinHigh && lowBattery)
            text = NTFY_TITLE_PIN_HIGH_LOW_BATTERY;
        else if (pinHigh)
            text = NTFY_TITLE_PIN_HIGH;
        else if (heartbeatTx && lowBattery)
            text = NTFY_TITLE_HEARTBEAT_TX_LOW_BATTERY;
        else if (heartbeatTx)
            text = NTFY_TITLE_HEARTBEAT_TX;
        else if (lowBattery)
            text = NTFY_TITLE_LOW_BATTERY;

        text += " (Vgpio = " + String(batteryLevel) + ")";

        return text;
    }

    /**
     * @brief Build notification title from board identifier.
     * @param jsonDoc JSON payload.
     * @return Board identifier (`board_id_hex`) when available.
     */
    String getNotificationTitle(const JsonDocument &jsonDoc) const
    {
        (void)jsonDoc;
        return "LoRa Mailbox";
    }

    /**
     * @brief Send an alert notification to NTFY via HTTPS.
     * @param jsonDoc JSON payload.
     * @param topic NTFY topic suffix.
     * @return true on HTTP 2xx success, false otherwise.
     */
    bool sendMsg(const JsonDocument &jsonDoc, const String &topic = NTFY_TOPIC)
    {
#if NTFY_ENABLED
        if (WiFi.status() != WL_CONNECTED)
            return false;

        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient https;
        String url = String(NTFY_SERVER) + topic;

        bool ok = false;
        if (!https.begin(client, url))
            return false;

        String title = getNotificationTitle(jsonDoc);
        String alertText = getNotificationText(jsonDoc);

        String message;
#if NTFY_INCLUDE_JSONL
        message = alertText;
        if (!message.isEmpty())
            message += "\n";
        if (!jsonDoc["jsonString"].isNull())
            message += String(jsonDoc["jsonString"].as<const char *>());
        else
        {
            String jsonPayload;
            serializeJson(jsonDoc, jsonPayload);
            message += jsonPayload;
        }
#else
        message = alertText;
#endif

        https.addHeader("Title", title);

        if (String(NTFY_USERNAME).length() > 0 || String(NTFY_PASSWORD).length() > 0)
            https.setAuthorization(NTFY_USERNAME, NTFY_PASSWORD);

        https.addHeader("Content-Type", "text/plain; charset=utf-8");
        int httpCode = https.POST(message);
        ok = (httpCode >= 200 && httpCode < 300);
        https.end();

        return ok;
#else
        return false;
#endif
    }
};
