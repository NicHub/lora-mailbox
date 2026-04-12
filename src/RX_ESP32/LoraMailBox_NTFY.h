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
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "user_settings/user_settings.h"

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
        if (!settings::ntfy::enabled)
            return NTFYMessageKind::None;

        const char *trigger = jsonDoc["TX"]["TX_TRIGGER"] | "";
        if (trigger[0] == '\0')
            trigger = jsonDoc["TX"]["TX_WAKEUP"] | "";
        if (trigger[0] == '\0')
            trigger = jsonDoc["TX"]["WAKEUP"] | "";
        if (trigger[0] == '\0')
            trigger = jsonDoc["TX"]["wakeup"] | "";
        if (strcmp(trigger, "WAKEUP_PIN_HIGH") == 0)
            return NTFYMessageKind::MessageReceived;
        if (settings::ntfy::notify_heartbeat_tx && strcmp(trigger, "HEARTBEAT_TX") == 0)
            return NTFYMessageKind::Heartbeat;
        return NTFYMessageKind::None;
    }

    String buildNTFYBody(const JsonDocument &jsonDoc) const
    {
        if (!settings::ntfy::enabled)
        {
            (void)jsonDoc;
            return "";
        }

        uint16_t counterValue = jsonDoc["COUNTER"]["VALUE"] | 0;
        const char *counterStatus = jsonDoc["COUNTER"]["STATUS"] | "";
        uint16_t tx_vbat_raw = jsonDoc["TX"]["TX_VBAT_RAW"] | 0;
        int tx_vbat_mv = jsonDoc["TX"]["TX_VBAT_MV"] | 0;
        int tx_vbat_percent = jsonDoc["TX"]["TX_VBAT_PERCENT"] | 0;
        const char *tx_vbat_glyph = jsonDoc["TX"]["TX_VBAT_GLYPH"] | "";
        const char *tx_vbat_status = jsonDoc["TX"]["TX_VBAT_STATUS"] | "";
        float rx_rssi_dbm = jsonDoc["RX"]["RX_RSSI_DBM"] | 0.0f;
        float rx_snr_db = jsonDoc["RX"]["RX_SNR_DB"] | 0.0f;
        const char *md_code_tag = settings::ntfy::md_code_tag;

        String text;
        text += md_code_tag;
        text += "bat ";
        text += tx_vbat_glyph;
        text += " ";
        text += String(tx_vbat_percent);
        text += "% (";
        text += String(tx_vbat_mv);
        text += "mV, ";
        text += tx_vbat_status;
        text += ", ";
        text += String(tx_vbat_raw);
        text += ")";
        text += md_code_tag;
        text += "\n";
        text += md_code_tag;
        text += "ctr ";
        text += String(counterValue);
        text += " (";
        text += counterStatus;
        text += ")";
        text += md_code_tag;
        text += "\n";
        text += md_code_tag;
        text += "RSSI ";
        text += String(rx_rssi_dbm, 0);
        text += "dBm ";
        text += " | ";
        text += "SNR ";
        text += String(rx_snr_db, 1);
        text += "dB";
        text += md_code_tag;

        return text;
    }

    NTFYMessage buildNTFYMessage(const JsonDocument &jsonDoc) const
    {
        if (!settings::ntfy::enabled)
        {
            (void)jsonDoc;
            return NTFYMessage{NTFYMessageKind::None, NTFYPriority::Default, "", ""};
        }

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
        title += " @" + String(settings::misc::recipient_name);
        title += ntfyTitleSuffix;

        return NTFYMessage{
            ntfyMessageKind,
            getNTFYPriority(ntfyMessageKind),
            title,
            buildNTFYBody(jsonDoc),
        };
    }

    bool sendMsg(const JsonDocument &jsonDoc, const String &topic = settings::ntfy::topic)
    {
        if (!settings::ntfy::enabled)
        {
            (void)jsonDoc;
            (void)topic;
            return false;
        }

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

        if (String(settings::ntfy::username).length() > 0 || String(settings::ntfy::password).length() > 0)
            https.setAuthorization(settings::ntfy::username, settings::ntfy::password);

        https.addHeader("Markdown", "yes");
        https.addHeader("Content-Type", "text/markdown; charset=utf-8");
        int httpCode = https.POST(ntfyMessage.body);
        ok = (httpCode >= 200 && httpCode < 300);
        https.end();

        return ok;
    }
};
