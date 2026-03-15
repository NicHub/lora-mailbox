/**
 * LoRa MailBox — NTFY
 *
 * https://ntfy.sh/
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "user_settings/user_settings.h"

#if NTFY_ENABLED
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>

class LoraMailBox_NTFY
{
public:
    LoraMailBox_NTFY() {}

    NotificationStatus evaluateNotificationStatus(const JsonDocument &jsonDoc) const
    {
        const char *wakeup = jsonDoc["WAKEUP"] | "";
        if (wakeup[0] == '\0')
            wakeup = jsonDoc["wakeup"] | "";
        if (strcmp(wakeup, "WAKEUP_PIN_HIGH") == 0)
            return NotificationStatus::MessageReceived;
#if NTFY_NOTIFY_HEARTBEAT_TX
        if (strcmp(wakeup, "HEARTBEAT_TX") == 0)
            return NotificationStatus::Heartbeat;
#endif
        return NotificationStatus::None;
    }

    /**
     * @brief Build notification emoji text from message categories.
     * @param jsonDoc JSON payload.
     * @param status Precomputed notification status.
     * @return Notification text.
     */
    String getNotificationText(const JsonDocument &jsonDoc, NotificationStatus status) const
    {
        (void)status;
        uint16_t counterValue = jsonDoc["COUNTER"]["VALUE"] | 0;
        const char *counterStatus = jsonDoc["COUNTER"]["STATUS"] | "";
        uint16_t vgpio = jsonDoc["VGPIO"] | 0;
        int vbat_mv = jsonDoc["VBAT_MV"] | 0;
        int vbat_percent = jsonDoc["VBAT_PERCENT"] | 0;
        const char *vbat_glyph = jsonDoc["VBAT_GLYPH"] | "";
        const char *vbat_status = jsonDoc["VBAT_STATUS"] | "";

        String text;
        text += "`   bat ";
        text += vbat_glyph;
        text += " ";
        text += String(vbat_percent);
        text += "%  (";
        text += String(vbat_mv);
        text += "mV, ";
        text += vbat_status;
        text += ", ";
        text += String(vgpio);
        text += ")`";
        text += "\n";
        text += "`   ctr ";
        text += String(counterValue);
        text += " (";
        text += counterStatus;
        text += ")`";

        return text;
    }

    /**
     * @brief Build notification title from board identifier.
     * @param jsonDoc JSON payload.
     * @param status Precomputed notification status.
     * @return Board identifier (`board_id_hex`) when available.
     */
    String getNotificationTitle(const JsonDocument &jsonDoc, NotificationStatus status) const
    {
        (void)jsonDoc;
        struct tm timeinfo;
        char timeStr[9] = "";
        if (getLocalTime(&timeinfo))
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

        String title;
        if (status == NotificationStatus::MessageReceived)
            title = String(NTFY_ICON_PIN_HIGH) + " ";
        else if (status == NotificationStatus::Heartbeat)
            title = String(NTFY_ICON_HEARTBEAT_TX) + " ";
        if (timeStr[0] != '\0')
            title += String(timeStr);
        title += " @" + String(NTFY_RECIPIENT_NAME);
        if (status == NotificationStatus::MessageReceived)
            title += " got mail";
        else if (status == NotificationStatus::Heartbeat)
            title += " got heartbeat";
        else
            title += " got undefined notif";


        return title;
    }

    /**
     * @brief Send an alert notification to NTFY via HTTPS.
     * @param jsonDoc JSON payload.
     * @param topic NTFY topic suffix.
     * @return true on HTTP 2xx success, false otherwise.
     */
    bool sendMsg(const JsonDocument &jsonDoc, const String &topic = NTFY_TOPIC)
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient https;
        String url = String(NTFY_SERVER) + topic;

        bool ok = false;
        if (!https.begin(client, url))
            return false;

        NotificationStatus status = evaluateNotificationStatus(jsonDoc);
        String title = getNotificationTitle(jsonDoc, status);
        String alertText = getNotificationText(jsonDoc, status);
        String message = alertText;

        https.addHeader("Title", title);

        if (String(NTFY_USERNAME).length() > 0 || String(NTFY_PASSWORD).length() > 0)
            https.setAuthorization(NTFY_USERNAME, NTFY_PASSWORD);

        https.addHeader("Markdown", "yes");
        https.addHeader("Content-Type", "text/markdown; charset=utf-8");
        int httpCode = https.POST(message);
        ok = (httpCode >= 200 && httpCode < 300);
        https.end();

        return ok;
    }
};

#else

class LoraMailBox_NTFY
{
public:
    LoraMailBox_NTFY() {}

    NotificationStatus evaluateNotificationStatus(const JsonDocument &jsonDoc) const
    {
        (void)jsonDoc;
        return NotificationStatus::None;
    }

    String getNotificationText(const JsonDocument &jsonDoc, NotificationStatus status) const
    {
        (void)jsonDoc;
        (void)status;
        return "";
    }

    String getNotificationTitle(const JsonDocument &jsonDoc, NotificationStatus status) const
    {
        (void)jsonDoc;
        (void)status;
        return "";
    }

    bool sendMsg(const JsonDocument &jsonDoc, const String &topic = "")
    {
        (void)jsonDoc;
        (void)topic;
        return false;
    }
};

#endif
