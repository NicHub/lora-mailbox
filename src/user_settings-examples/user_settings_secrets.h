/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

namespace settings::wifi
{
    struct Credentials
    {
        const char *ssid;
        const char *password;
    };

    /** @brief Declare any number of WiFi networks. RX either tries them in this exact order or
     * picks the strongest visible one, depending on CONNECTION_PRIORITY. */
    static constexpr Credentials NETWORKS[] = {
        {"ssid example 0", "password example 0"},
        {"ssid example 1", "password example 1"},
    };
}

namespace settings::mqtt
{
    static constexpr char USERNAME[] = "guest";
    static constexpr char PASSWORD[] = "guest123";
#include "mqtt_root_ca.pem.inc"
}

namespace settings::ntfy
{
    static constexpr char USERNAME[] = "rolf";
    static constexpr char PASSWORD[] = "ntfyntfy1234";
}
