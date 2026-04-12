# XIAO nRF52840 Sense Hardware Notes

## LEDs

The Seeed Studio XIAO nRF52840 Sense exposes:

- one user RGB LED
- one battery charge LED

### Summary Table

| LED    | Role                              | nRF52840 GPIO | Net / label in schematic | LED Macro                             | Notes                |
| ------ | --------------------------------- | ------------- | ------------------------ | ------------------------------------- | -------------------- |
| Red    | Red channel of the user RGB LED   | `P0.26`       | `USER_RED`               | `LED_RED` = `LED_BUILTIN` = `PIN_LED` | User LED             |
| Green  | Green channel of the user RGB LED | `P0.30`       | `USER_GREEN`             | `LED_GREEN`                           | User LED             |
| Blue   | Blue channel of the user RGB LED  | `P0.06`       | `USER_BLUE`              | `LED_BLUE`                            | User LED             |
| Charge | Battery charge status indicator   | `P0.17`       | `CHG_LED`                |                                       | Dedicated charge LED |

### Notes

- The RGB LED is driven through three separate outputs: red, green, and blue.
- The charge LED is separate from the user RGB LED.
- The net names `USER_RED`, `USER_GREEN`, `USER_BLUE`, and `CHG_LED` are taken from the board documentation / schematic.
- `CHG_LED` should be treated as a charger status signal, not as a general-purpose firmware-controlled LED.
- `P0.17` is tied to the charger status path and should be used as an input signal if charge state detection is needed.

### Arduino Variant Reference

- `$HOME/.platformio/packages/framework-arduinoadafruitnrf52@src-bd3e1c9fce4039a1b5606180647d320c/variants/Seeed_XIAO_nRF52840/variant.h`
- This variant file defines the board-level LED macros such as `LED_RED`, `LED_GREEN`, `LED_BLUE`, `LED_BUILTIN`, and `PIN_LED`.

## Battery Voltage Reading

`VBAT_ENABLE` controls the battery voltage divider used to read `PIN_VBAT`.

Reference:

- Seeed Studio Wiki: [XIAO nRF52840 battery charging considerations]

### `VBAT_ENABLE` behavior

| `pinMode(VBAT_ENABLE)` | `digitalWrite(VBAT_ENABLE)` | Safe | Intended usage        | Effect                                                       |
| ---------------------- | --------------------------- | ---- | --------------------- | ------------------------------------------------------------ |
| `INPUT`                | `LOW`                       | Yes  | Low power consumption | Divider off, high impedance                                  |
| `INPUT`                | `HIGH`                      | Yes  | Low power consumption | Divider off, high impedance                                  |
| `OUTPUT`               | `LOW`                       | Yes  | Battery read          | Divider on, `VBAT` readable on `PIN_VBAT`                    |
| `OUTPUT`               | `HIGH`                      | No   | Avoid                 | Forces `3.3V`, may exceed the expected voltage on `PIN_VBAT` |

### Recommended sequence

```cpp
pinMode(VBAT_ENABLE, OUTPUT);
digitalWrite(VBAT_ENABLE, LOW); // enable divider
int vbat = analogRead(PIN_VBAT);
pinMode(VBAT_ENABLE, INPUT);    // disable divider
```

### Implementation note

- `readBatteryVoltage()` enables the divider, waits for the analog node to settle, discards the first sample, averages several ADC reads, then restores `VBAT_ENABLE` to `INPUT` to minimize current draw.

### Sources

- [XIAO – nRF52840 – schematics (PDF)]
- [XIAO – LoRa SX1262 shield – schematics (PDF)]
- [XIAO – nRF52840 – Seeed Studio Wiki]

[XIAO nRF52840 battery charging considerations]: https://wiki.seeedstudio.com/XIAO_BLE/#q3-what-are-the-considerations-when-using-xiao-nrf52840-sense-for-battery-charging
[XIAO – nRF52840 – schematics (PDF)]: ./Seeed-Studio-XIAO-nRF52840-Sense-v1.1.pdf
[XIAO – LoRa SX1262 shield – schematics (PDF)]: ./Schematic_Diagram_Wio-SX1262_for_XIAO.pdf
[XIAO – nRF52840 – Seeed Studio Wiki]: https://wiki.seeedstudio.com/XIAO_BLE/
