/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-one-later, Nicolas Jeanmonod, ouilogique.com
 */

#pragma once

namespace settings::wifi
{
static constexpr char SSID[] = "";
static constexpr char PASSWORD[] = "";
} // namespace settings::wifi

namespace settings::mqtt
{
static constexpr char USERNAME[] = "guest";
static constexpr char PASSWORD[] = "guest123";
#include "mqtt_root_ca.pem.inc"
} // namespace settings::mqtt

namespace settings::ntfy
{
static constexpr char USERNAME[] = "rolf";
static constexpr char PASSWORD[] = "ntfyntfy1234";
} // namespace settings::ntfy
