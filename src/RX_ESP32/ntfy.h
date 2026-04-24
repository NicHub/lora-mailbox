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
#include "common/ntfy_types.h"
#include "user_settings/user_settings.h"

class LoraMailboxNtfy
{
  public:
    LoraMailboxNtfy()
    {
    }

    NTFYPriority getNTFYPriority(TxTrigger tx_trigger) const;
    const char *getNTFYIcon(TxTrigger tx_trigger) const;
    const char *getNTFYTitleSuffix(TxTrigger tx_trigger) const;
    TxTrigger getNTFYTrigger(const JsonDocument &json_doc) const;
    String getConnectedWifiSsid(const JsonDocument &json_doc) const;
    String buildNTFYBody(const JsonDocument &json_doc) const;
    NTFYMessage buildNTFYMessage(const JsonDocument &json_doc) const;
    bool sendMsg(const JsonDocument &json_doc, const String &topic = settings::ntfy::TOPIC);
};
