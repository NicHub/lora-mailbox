###
# Go in the fonts directory
# Edit this file to change the font path according
##
import subprocess
import os


fontconvert = (
    '"/Users/nico/kdnicomac/PlatformIO/Projects/esp32-weather-station/.pio/libdeps/lolin_s2_mini/Adafruit GFX Library/fontconvert/fontconvert"'
)
if not os.path.exists(fontconvert):
    print(f"{fontconvert} cannot be found")
    SystemExit(f"{fontconvert} cannot be found")
else:
    print(f"{fontconvert} found")
    SystemExit(f"{fontconvert} found")


fonts = [
    {"path": "/System/Library/Fonts/Supplemental/Comic Sans MS Bold.ttf", "size": 13},
]

preamble = "#pragma once\n#include <Adafruit_GFX.h>\n\n"

for font in fonts:
    cmd = f'{fontconvert} "{font["path"]}" {font["size"]}'
    ans = subprocess.run(
        args=cmd, shell=True, capture_output=True, check=False, encoding="utf-8"
    )

    for line in ans.stdout.splitlines():
        pos1 = line.find("GFXfont")
        if pos1 == -1:
            continue
        pos2 = line.find("PROGMEM")
        if pos2 == -1:
            continue
        font_name = line[pos1 + len("GFXfont") + 1 : pos2 - 1] + ".h"
        break

    print(f'#include "../fonts/{font_name}"')
    with open(file=font_name, mode="wt", encoding="utf-8") as _f:
        _f.write(preamble)
        _f.write(ans.stdout)

print("\n\n")
