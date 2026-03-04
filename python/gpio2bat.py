"""

Vbat calc = 3985.8
Vbat mes  = 3900

"""


def Vgpio2Vbat(Vgpio_meas):
    a = 11.187
    b = -187
    Vbat_calc_mV = a * Vgpio_meas + b
    return Vbat_calc_mV


def Vgpio2Vbat_2(Vgpio_meas):
    a = 10
    b = +200
    Vbat_calc_mV = a * Vgpio_meas + b
    return Vbat_calc_mV


if __name__ == "__main__":
    Vgpio_meas_s = [
        373,
        353,
        400,
    ]
    for Vgpio_meas in Vgpio_meas_s:
        Vbat_calc_mV_1 = Vgpio2Vbat(Vgpio_meas)
        print(f"Vgpio_meas: {Vgpio_meas}, Vbat_calc_mV_1: {Vbat_calc_mV_1:0.0f}")
        Vbat_calc_mV_2 = Vgpio2Vbat_2(Vgpio_meas)
        print(f"Vgpio_meas: {Vgpio_meas}, Vbat_calc_mV_2: {Vbat_calc_mV_2:0.0f}")
        print()
