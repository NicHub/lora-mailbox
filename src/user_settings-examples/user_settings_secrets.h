/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

#define NTFY_USERNAME ""
#define NTFY_PASSWORD ""

static const char MQTT_ROOT_CA_PEM[] = R"PEM(-----BEGIN CERTIFICATE-----
YOUR_CA_CERTIFICATE_HERE
-----END CERTIFICATE-----
)PEM";
