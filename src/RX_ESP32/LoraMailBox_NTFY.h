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
#include "common/LoraMailBox_NTFY_types.h"
#include "user_settings/user_settings.h"

class LoraMailboxNtfy
{
public:
    LoraMailboxNtfy() {}

    NTFYPriority getNTFYPriority(TxTrigger tx_trigger) const
    {
        switch (tx_trigger)
        {
        case TxTrigger::WakeupPinHigh:
            return settings::ntfy::message_received_priority;
        case TxTrigger::HeartbeatTx:
            return settings::ntfy::heartbeat_priority;
        case TxTrigger::Boot:
            return settings::ntfy::boot_priority;
        default:
            return NTFYPriority::Default;
        }
    }

    const char *getNTFYIcon(TxTrigger tx_trigger) const
    {
        switch (tx_trigger)
        {
        case TxTrigger::WakeupPinHigh:
            return settings::ntfy::message_received_icon;
        case TxTrigger::HeartbeatTx:
            return settings::ntfy::heartbeat_icon;
        case TxTrigger::Boot:
            return settings::ntfy::boot_icon;
        default:
            return "";
        }
    }

    const char *getNTFYTitleSuffix(TxTrigger tx_trigger) const
    {
        switch (tx_trigger)
        {
        case TxTrigger::WakeupPinHigh:
            return settings::ntfy::message_received_title_suffix;
        case TxTrigger::HeartbeatTx:
            return settings::ntfy::heartbeat_title_suffix;
        case TxTrigger::Boot:
            return settings::ntfy::boot_title_suffix;
        default:
            return "";
        }
    }

    TxTrigger getNTFYTrigger(const JsonDocument &json_doc) const
    {
        if (!settings::ntfy::enabled)
            return TxTrigger::Boot;

        const char *tx_trigger = json_doc["TX"]["TX_TRIGGER"] | "";
        if (strcmp(tx_trigger, "WAKEUP_PIN_HIGH") == 0)
            return TxTrigger::WakeupPinHigh;
        if (settings::ntfy::notify_heartbeat_tx && strcmp(tx_trigger, "HEARTBEAT_TX") == 0)
            return TxTrigger::HeartbeatTx;
        return TxTrigger::Boot;
    }

    String buildNTFYBody(const JsonDocument &json_doc) const
    {
        if (!settings::ntfy::enabled)
        {
            (void)json_doc;
            return "";
        }

        uint16_t counter_value = json_doc["RX"]["RX_COUNTER"] | 0;
        const char *counter_status = json_doc["RX_TX"]["RX_TX_COUNTER_STATUS"] | "";
        uint16_t tx_vbat_raw = json_doc["TX"]["TX_VBAT_RAW"] | 0;
        int tx_vbat_mv = json_doc["TX"]["TX_VBAT_MV"] | 0;
        int tx_vbat_percent = json_doc["TX"]["TX_VBAT_PERCENT"] | 0;
        const char *tx_vbat_glyph = json_doc["TX"]["TX_VBAT_GLYPH"] | "";
        const char *tx_vbat_status = json_doc["TX"]["TX_VBAT_STATUS"] | "";
        float rx_tx_rssi_dbm = json_doc["RX_TX"]["RX_TX_RSSI_DBM"] | 0.0f;
        float rx_tx_snr_db = json_doc["RX_TX"]["RX_TX_SNR_DB"] | 0.0f;
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
        text += String(counter_value);
        text += " (";
        text += counter_status;
        text += ")";
        text += md_code_tag;
        text += "\n";
        text += md_code_tag;
        text += "RSSI ";
        text += String(rx_tx_rssi_dbm, 0);
        text += "dBm ";
        text += " | ";
        text += "SNR ";
        text += String(rx_tx_snr_db, 1);
        text += "dB";
        text += md_code_tag;

        return text;
    }

    NTFYMessage buildNTFYMessage(const JsonDocument &json_doc) const
    {
        if (!settings::ntfy::enabled)
        {
            (void)json_doc;
            return NTFYMessage{TxTrigger::Boot, NTFYPriority::Default, "", ""};
        }

        TxTrigger tx_trigger = getNTFYTrigger(json_doc);

        struct tm timeinfo;
        char time_str[9] = "";
        if (getLocalTime(&timeinfo))
            strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);

        const char *ntfy_icon = getNTFYIcon(tx_trigger);
        const char *ntfy_title_suffix = getNTFYTitleSuffix(tx_trigger);
        String title;
        if (ntfy_icon[0] != '\0')
            title = String(ntfy_icon) + " ";
        if (time_str[0] != '\0')
            title += String(time_str);
        title += " @" + String(settings::misc::recipient_name);
        title += ntfy_title_suffix;

        return NTFYMessage{
            tx_trigger,
            getNTFYPriority(tx_trigger),
            title,
            buildNTFYBody(json_doc),
        };
    }

    bool sendMsg(const JsonDocument &json_doc, const String &topic = settings::ntfy::topic)
    {
        if (!settings::ntfy::enabled)
        {
            (void)json_doc;
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

        NTFYMessage ntfy_message = buildNTFYMessage(json_doc);
        https.addHeader("Title", ntfy_message.title);
        https.addHeader("Priority", ntfyPriorityToString(ntfy_message.priority));

        if (String(settings::ntfy::username).length() > 0 || String(settings::ntfy::password).length() > 0)
            https.setAuthorization(settings::ntfy::username, settings::ntfy::password);

        https.addHeader("Markdown", "yes");
        https.addHeader("Content-Type", "text/markdown; charset=utf-8");
        int http_code = https.POST(ntfy_message.body);
        ok = (http_code >= 200 && http_code < 300);
        https.end();

        return ok;
    }
};
