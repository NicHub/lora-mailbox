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

    NTFYPriority getNTFYPriority(NTFYMessageKind ntfyMessageKind) const
    {
        switch (ntfyMessageKind)
        {
        case NTFYMessageKind::MessageReceived:
            return settings::ntfy::message_received_priority;
        case NTFYMessageKind::Heartbeat:
            return settings::ntfy::heartbeat_priority;
        case NTFYMessageKind::None:
        default:
            return NTFYPriority::Default;
        }
    }

    const char *getNTFYIcon(NTFYMessageKind ntfyMessageKind) const
    {
        switch (ntfyMessageKind)
        {
        case NTFYMessageKind::MessageReceived:
            return settings::ntfy::message_received_icon;
        case NTFYMessageKind::Heartbeat:
            return settings::ntfy::heartbeat_icon;
        case NTFYMessageKind::None:
        default:
            return "";
        }
    }

    const char *getNTFYTitleSuffix(NTFYMessageKind ntfyMessageKind) const
    {
        switch (ntfyMessageKind)
        {
        case NTFYMessageKind::MessageReceived:
            return settings::ntfy::message_received_title_suffix;
        case NTFYMessageKind::Heartbeat:
            return settings::ntfy::heartbeat_title_suffix;
        case NTFYMessageKind::None:
        default:
            return "";
        }
    }

    NTFYMessageKind getNTFYMessageKind(const JsonDocument &jsonDoc) const
    {
        const char *wakeup = jsonDoc["WAKEUP"] | "";
        if (wakeup[0] == '\0')
            wakeup = jsonDoc["wakeup"] | "";
        if (strcmp(wakeup, "WAKEUP_PIN_HIGH") == 0)
            return NTFYMessageKind::MessageReceived;
        if (settings::ntfy::notify_heartbeat_tx && strcmp(wakeup, "HEARTBEAT_TX") == 0)
            return NTFYMessageKind::Heartbeat;
        return NTFYMessageKind::None;
    }

    String buildNTFYBody(const JsonDocument &jsonDoc) const
    {
        uint16_t counterValue = jsonDoc["COUNTER"]["VALUE"] | 0;
        const char *counterStatus = jsonDoc["COUNTER"]["STATUS"] | "";
        uint16_t vgpio = jsonDoc["VGPIO"] | 0;
        int vbat_mv = jsonDoc["VBAT_MV"] | 0;
        int vbat_percent = jsonDoc["VBAT_PERCENT"] | 0;
        const char *vbat_glyph = jsonDoc["VBAT_GLYPH"] | "";
        const char *vbat_status = jsonDoc["VBAT_STATUS"] | "";

        String text;
        text += "`bat ";
        text += vbat_glyph;
        text += " ";
        text += String(vbat_percent);
        text += "% (";
        text += String(vbat_mv);
        text += "mV, ";
        text += vbat_status;
        text += ", ";
        text += String(vgpio);
        text += ")`";
        text += "\n";
        text += "`ctr ";
        text += String(counterValue);
        text += " (";
        text += counterStatus;
        text += ")`";

        return text;
    }

    /**
     * @brief Build a complete NTFY message from the current payload.
     */
    NTFYMessage buildNTFYMessage(const JsonDocument &jsonDoc) const
    {
        NTFYMessageKind ntfyMessageKind = getNTFYMessageKind(jsonDoc);
        if (ntfyMessageKind == NTFYMessageKind::None)
            return NTFYMessage{NTFYMessageKind::None, NTFYPriority::Default, "", ""};

        struct tm timeinfo;
        char timeStr[9] = "";
        if (getLocalTime(&timeinfo))
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

        const char *ntfyIcon = getNTFYIcon(ntfyMessageKind);
        const char *ntfyTitleSuffix = getNTFYTitleSuffix(ntfyMessageKind);
        String title;
        if (ntfyIcon[0] != '\0')
            title = String(ntfyIcon) + " ";
        if (timeStr[0] != '\0')
            title += String(timeStr);
        title += " @" + String(settings::mailbox::recipient_name);
        title += ntfyTitleSuffix;

        return NTFYMessage{
            ntfyMessageKind,
            getNTFYPriority(ntfyMessageKind),
            title,
            buildNTFYBody(jsonDoc),
        };
    }

    /**
     * @brief Send an alert notification to NTFY via HTTPS.
     * @param jsonDoc JSON payload.
     * @param topic NTFY topic suffix.
     * @return true on HTTP 2xx success, false otherwise.
     */
    bool sendMsg(const JsonDocument &jsonDoc, const String &topic = settings::ntfy::topic)
    {
        if (WiFi.status() != WL_CONNECTED)
            return false;

        WiFiClientSecure client;
        client.setInsecure();

        HTTPClient https;
        String url = String(settings::ntfy::server) + topic;

        bool ok = false;
        if (!https.begin(client, url))
            return false;

        NTFYMessage ntfyMessage = buildNTFYMessage(jsonDoc);
        if (ntfyMessage.kind == NTFYMessageKind::None)
        {
            https.end();
            return false;
        }

        https.addHeader("Title", ntfyMessage.title);
        https.addHeader("Priority", ntfyPriorityToString(ntfyMessage.priority));

        if (String(NTFY_USERNAME).length() > 0 || String(NTFY_PASSWORD).length() > 0)
            https.setAuthorization(NTFY_USERNAME, NTFY_PASSWORD);

        https.addHeader("Markdown", "yes");
        https.addHeader("Content-Type", "text/markdown; charset=utf-8");
        int httpCode = https.POST(ntfyMessage.body);
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

    NTFYMessageKind getNTFYMessageKind(const JsonDocument &jsonDoc) const
    {
        (void)jsonDoc;
        return NTFYMessageKind::None;
    }

    String buildNTFYBody(const JsonDocument &jsonDoc) const
    {
        (void)jsonDoc;
        return "";
    }

    NTFYMessage buildNTFYMessage(const JsonDocument &jsonDoc) const
    {
        (void)jsonDoc;
        return NTFYMessage{NTFYMessageKind::None, NTFYPriority::Default, "", ""};
    }

    bool sendMsg(const JsonDocument &jsonDoc, const String &topic = "")
    {
        (void)jsonDoc;
        (void)topic;
        return false;
    }
};

#endif
