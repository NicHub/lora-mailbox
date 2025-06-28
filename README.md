# LoRa MailBox

Detects when mailbox flap is opened using a PIR motion sensor and sends a notification via LoRa.

## Before you start

-   Rename `src/RX/credentials-example.h` to `src/RX/credentials.h`
    and and update the details with your information.
-   Rename `platformio_user_preferences-example.ini` to `platformio_user_preferences.ini`
    and and update the details with your information.

## Hardware

-   XIAO ESP32S3 & Wio-SX1262 Kit

    -   https://wiki.seeedstudio.com/wio_sx1262_with_xiao_esp32s3_kit/
    -   https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
    -   https://wiki.seeedstudio.com/xiao_esp32s3_&_wio_SX1262_kit_for_meshtastic/

-   PIR AM312

    -   https://fr.aliexpress.com/item/1005008067324301.html

-   3.7 V Lipo Battery

    -   https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/#battery-usage
    -   https://wiki.seeedstudio.com/check_battery_voltage/

## Software

-   PlatformIO on VSCode

    -   https://code.visualstudio.com/
    -   https://platformio.org/

## Usefull commands

```zsh
# The ESP32S3 is blocked very often.
alias esptool.py='$HOME/.platformio/packages/tool-esptoolpy/esptool.py'
esptool.py --chip esp32s3 --port $PORT erase_flash

```

## License

Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
