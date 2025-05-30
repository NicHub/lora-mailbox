# LoRa MailBox

Detects when mailbox flap is opened using a PIR motion sensor and sends a notification via LoRa.

## Hardware

-   XIAO ESP32S3 & Wio-SX1262 Kit

    -   https://wiki.seeedstudio.com/wio_sx1262_with_xiao_esp32s3_kit/
    -   https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
    -   https://wiki.seeedstudio.com/xiao_esp32s3_&_wio_SX1262_kit_for_meshtastic/

-   PIR AM312

    -   https://fr.aliexpress.com/item/1005008067324301.html

-   3.7 V Lipo Battery

    -   https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/#battery-usage

## Software

-   PlatformIO on VSCode

    -   https://code.visualstudio.com/
    -   https://platformio.org/

## Usefull commands

```zsh

esptool.py --chip esp32s3 --port /dev/cu.usbmodem11101 erase_flash

```

# License

Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
