/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

namespace settings::wifi
{
    static constexpr char ssid[] = "";
    static constexpr char password[] = "";
}

namespace settings::mqtt
{
    static constexpr char username[] = "guest";
    static constexpr char password[] = "guest123";
#include "mqtt_root_ca.pem.inc"
}

namespace settings::ntfy
{
    static constexpr char username[] = "rolf";
    static constexpr char password[] = "ntfyntfy1234";
}
