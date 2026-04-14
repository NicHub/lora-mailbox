# LoRa MailBox

Detects when mailbox flap is opened using a PIR motion sensor and sends a notification via LoRa.

## Before you start

TODO: Update this section

-   ~~The first build copies files from `src/user_settings-examples/` to `src/user_settings/` if they are missing.~~
-   ~~Update the files in `src/user_settings/` with your information.~~
-   ~~`src/user_settings/` is intentionally a nested local Git repository used to keep private machine-specific settings and secrets out of the main project repository.~~
-   ~~Keep `src/user_settings-examples/` and `src/user_settings/` structurally aligned, but never copy secrets from `src/user_settings/` into the example directory.~~
-   ~~Copy `user_settings-example.ini` to `user_settings.ini`~~
    ~~and update the serial port details for your machine.~~

## Hardware

-   XIAO ESP32S3 & Wio-SX1262 Kit

    -   https://wiki.seeedstudio.com/wio_sx1262_with_xiao_esp32s3_kit/
    -   https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
    -   https://wiki.seeedstudio.com/xiao_esp32s3_&_wio_SX1262_kit_for_meshtastic/
    -   https://files.seeedstudio.com/products/SenseCAP/Wio_SX1262/Schematic_Diagram_Wio-SX1262_for_XIAO.pdf

-   XIAO nRF52840 & Wio-SX1262 Kit

    -   https://github.com/meshtastic/firmware/blob/master/variants/nrf52840/seeed_xiao_nrf52840_kit/variant.h
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

## Code style

This project follows the [`readability-identifier-naming`](https://clang.llvm.org/extra/clang-tidy/checks/readability/identifier-naming.html) conventions enforced by clang-tidy (see `scripts/tools/.clang-tidy`).

To check: `./scripts/tools/run_clang_tidy.sh [rx|tx|all]`

## LoRa settings

The LoRa settings are defined in:

- `src/user_settings/user_settings.h`
- `src/user_settings-examples/user_settings.h`

### Parameter guide

- `FREQ`
  Carrier frequency in MHz.
  SX1262 capability: `150.0` to `960.0` MHz.
  Legal and usable frequencies depend on the target region and local regulations.
  Impact: changing this mainly changes regulatory compliance, propagation characteristics, and interference exposure.
  Default in `SX1262.h`: `434.0`.

- `BW`
  LoRa bandwidth in kHz.
  Typical values: `10.4`, `15.6`, `20.8`, `31.25`, `41.7`, `62.5`, `125`, `250`, `500`.
  `BW ↑ => throughput ↑, airtime ↓, sensitivity ↓, robustness in difficult environments ↓`
  `BW ↓ => throughput ↓, airtime ↑, sensitivity ↑, robustness in difficult environments ↑`
  Default in `SX1262.h`: `125.0`.

- `SF`
  LoRa spreading factor. Valid range: `6` to `12`.
  `SF ↑ => throughput ↓, airtime ↑, sensitivity ↑, range/penetration ↑, battery cost per message ↑`
  `SF ↓ => throughput ↑, airtime ↓, sensitivity ↓, range/penetration ↓, battery cost per message ↓`
  Default in `SX1262.h`: `9`.

- `CR`
  LoRa coding rate denominator. Valid range: `5` to `8`.
  `CR ↑ => redundancy ↑, airtime ↑, throughput ↓, resilience to errors/interference ↑`
  `CR ↓ => redundancy ↓, airtime ↓, throughput ↑, resilience to errors/interference ↓`
  Default in `SX1262.h`: `7`.

- `SYNCWORD`
  LoRa sync word used to distinguish networks.
  `0x34` is reserved for LoRaWAN.
  Impact: changes network separation only; it does not directly improve range, speed, or power use.
  Default in `SX1262.h`: `RADIOLIB_SX126X_SYNC_WORD_PRIVATE`.

- `POWER`
  TX output power in dBm.
  Typical SX1262 range: `2` to `17` dBm.
  `POWER ↑ => link budget ↑, range margin ↑, battery usage ↑, legal-risk/EMI risk ↑`
  `POWER ↓ => link budget ↓, range margin ↓, battery usage ↓`
  Default in `SX1262.h`: `10`.

- `PREAMBLE_LENGTH`
  Preamble length in symbols.
  The effective transmitted preamble is `4.25` symbols longer.
  `PREAMBLE_LENGTH ↑ => sync reliability ↑, airtime ↑, throughput ↓`
  `PREAMBLE_LENGTH ↓ => sync reliability ↓, airtime ↓, throughput ↑`
  Default in `SX1262.h`: `8`.

- `TCXO_VOLTAGE`
  TCXO control voltage in V.
  Used by `radio.begin()` on boards that drive the SX1262 TCXO.
  Impact: this is a hardware-matching parameter, not a tuning knob for speed or range.
  Wrong value can cause startup failure, unstable RF behavior, or no radio link.
  Default in `SX1262.h`: `1.6`.

- `USE_REGULATOR_LDO`
  Use the SX1262 LDO regulator instead of DC-DC.
  `USE_REGULATOR_LDO = true => simplicity/compatibility may ↑, efficiency usually ↓`
  `USE_REGULATOR_LDO = false => efficiency usually ↑, but depends on board design and stability`
  Default in `SX1262.h`: `false`.

## Usefull commands

```zsh
# The ESP32S3 is blocked very often.
alias esptool.py='$HOME/.platformio/packages/tool-esptoolpy/esptool.py'
esptool.py --chip esp32s3 --port $PORT erase_flash

```

## Troubleshooting

Mac with ARM architecture can face the following error:

```
Adafruit_TinyUSB_Arduino arm-none-eabi-g++: Bad CPU type in executable
```

This can be solved by installing Rosetta:

```
softwareupdate --install-rosetta --agree-to-license
```

If the VS Code linter does not recognize project include paths or board constants such as `LED_RED`:

1. Open `user_settings.ini`.
2. Check `default_envs`.
3. Make sure `seeed_xiao_nrf52840-tx` is the first entry in `default_envs`.

## Videos

-   <https://youtu.be/mFGQBNXpCBE>

## See also

-   https://github.com/PricelessToolkit/PirBOX-LITE.git

## License

Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
