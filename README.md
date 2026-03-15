# LoRa MailBox

Detects when mailbox flap is opened using a PIR motion sensor and sends a notification via LoRa.

## Before you start

-   The first build copies files from `src/user_settings-examples/` to `src/user_settings/` if they are missing.
-   Update the files in `src/user_settings/` with your information.
-   Copy `user_settings-example.ini` to `user_settings.ini`
    and update the serial port details for your machine.

## Hardware

-   XIAO ESP32S3 & Wio-SX1262 Kit

    -   https://wiki.seeedstudio.com/wio_sx1262_with_xiao_esp32s3_kit/
    -   https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
    -   https://wiki.seeedstudio.com/xiao_esp32s3_&_wio_SX1262_kit_for_meshtastic/
    -   https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Schematic_Diagram_Wio-SX1262_for_XIAO.pdf

-   XIAO nRF52840 & Wio-SX1262 Kit

    -   https://github.com/meshtastic/firmware/blob/master/variants/seeed_xiao_nrf52840_kit/variant.h
    -   https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Wio-SX1262%20for%20XIAO%20V1.0_SCH.pdf
    -   Wio-SX1262 for XIAO standalone SKU `113010003`
    -   XIAO nRF52840 kit SKU `102010710`

-   PIR AM312

    -   https://fr.aliexpress.com/item/1005008067324301.html
    -   https://www.image.micros.com.pl/_dane_techniczne_auto/cz%20am312.pdf?utm_source=chatgpt.com
    -   ./docs/AM312-1.pdf
    -   ./docs/AM312-2.pdf

-   3.7 V Lipo Battery

    -   https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/#battery-usage
    -   https://wiki.seeedstudio.com/check_battery_voltage/

## Software

-   PlatformIO on VSCode

    -   https://code.visualstudio.com/
    -   https://platformio.org/

-   Python tools dependencies

    -   `python3 -m pip install -r requirements.txt`

## Usefull commands

```zsh
# The ESP32S3 is blocked very often.
alias esptool.py='$HOME/.platformio/packages/tool-esptoolpy/esptool.py'
esptool.py --chip esp32s3 --port $PORT erase_flash

```

## Troubleshooting

Mac with ARM architecture can face the following error:

```
Adafruit_TinyUSB_Arduino arm-none-eabi-g++: Bad CPU type in executable
```

This can be solved by installing Rosetta:

```
softwareupdate --install-rosetta --agree-to-license
```

## See also

-   https://github.com/PricelessToolkit/PirBOX-LITE.git

## License

Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
