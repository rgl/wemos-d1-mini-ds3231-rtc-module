
# About

![Build](https://github.com/rgl/wemos-d1-mini-esp8266-ds3231-rtc-module/workflows/Build/badge.svg)

This shows a way to turn on/off a load using a relay module connected to a WeMos D1 ESP8266 board.

These instructions apply to the WeMos D1 mini boards:

* [WeMos D1 mini](https://docs.wemos.cc/en/latest/d1/d1_mini.html)
* [WeMos D1 mini Pro](https://www.wemos.cc/en/latest/d1/d1_mini_pro.html)

To complete this project you'll need:

* 3 independent power supplies
  * 220vac to 5vdc to power the relay module
  * 220vac to 5vdc to power the WeMos board
  * 220vac to 24vac to power the load (e.g. a [Rain Bird 100-JTV-FF valve](https://www.rainbird.eu/products/valves/jar-top-series-valve-jtv))
* 1 relay module
* 1 WeMos D1 mini board

This board connects to the computer as a USB serial device, as such, you should add your user to the `dialout` group:

```bash
# add ourselfs to the dialout group to be able to write to the USB serial
# device without root permissions.
sudo usermod -a -G dialout $USERNAME
# enter a new shell with the dialout group enabled.
# NB to make this permanent its easier to reboot the system with sudo reboot
#    or to logout/login your desktop.
newgrp dialout
```

Create a python virtual environment in the current directory:

```bash
sudo apt-get install -y --no-install-recommends python3-pip python3-venv
python3 -m venv venv
```

Install [esptool](https://github.com/espressif/esptool):

```bash
source ./venv/bin/activate
# NB this pip install will display several "error: invalid command 'bdist_wheel'"
#    messages, those can be ignored.
# see https://pypi.org/project/esptool/
# see https://github.com/espressif/esptool
# TODO use a requirements.txt file and then python3 -m pip install -r requirements.txt
esptool_version='2.8'
python3 -m pip install esptool==$esptool_version
esptool.py version
```

Dump the information about the esp attached to the serial port:

```bash
esptool.py -p /dev/ttyUSB0 chip_id
```

You should see something alike:

```plain
esptool.py v2.8
Serial port /dev/ttyUSB0
Connecting....
Detecting chip type... ESP8266
Chip is ESP8266EX
Features: WiFi
Crystal is 26MHz
MAC: 5c:cf:7f:c5:f4:8e
Uploading stub...
Running stub...
Stub running...
Chip ID: 0x00c5f48e
Hard resetting via RTS pin...
```

Dump the information about the flash:

```bash
esptool.py -p /dev/ttyUSB0 flash_id
```

You should see something alike:

```plain
esptool.py v2.8
Serial port /dev/ttyUSB0
Connecting....
Detecting chip type... ESP8266
Chip is ESP8266EX
Features: WiFi
Crystal is 26MHz
MAC: 5c:cf:7f:c5:f4:8e
Uploading stub...
Running stub...
Stub running...
Manufacturer: ef
Device: 4016
Detected flash size: 4MB
Hard resetting via RTS pin...
```

This flash device Manufacturer and Device ID is described at [coreboot flashrom repository](https://review.coreboot.org/cgit/flashrom.git/tree/flashchips.h), in this case, its:

```c
#define WINBOND_NEX_ID          0xEF    /* Winbond (ex Nexcom) serial flashes */
#define WINBOND_NEX_W25Q32_V	0x4016	/* W25Q32BV; W25Q32FV in SPI mode (default) */
```

This SPI flash supports QPI mode. Since the WeMos board has connected all flash pins to the ESP we can use the qio flash mode.

## platformio

Install platformio:

```bash
# active our isolated python environment.
source ./venv/bin/activate
# install platformio.
# see https://docs.platformio.org/en/latest/core/installation.html
# see https://docs.platformio.org/en/latest/projectconf/section_platformio.html#core-dir
# see https://pypi.org/project/platformio/
export PLATFORMIO_CORE_DIR="$VIRTUAL_ENV/platformio"
python3 -m pip install platformio==4.3.4
platformio --version
```

## RTC Module

When using the board without internet we need a Real-Time-Clock (RTC) time module like one with a DS3231M chip.

To use the RTC module, build Tasmota from source as:

```bash
# active our isolated python environment.
source ./venv/bin/activate
platformio run
firmware_image=.pio/build/d1_mini/firmware.bin
ls -laF $firmware_image
# flash the device.
# NB you should add the `--erase-all` argument the first time you upload the
#    firmware (if a different firmware was on the device).
esptool.py -p /dev/ttyUSB0 write_flash --flash_mode qio 0x0 $firmware_image
# transform the data sub-directory contents into a filesystem image
# and flash it to the device.
platformio run --target uploadfs
```

## web ui

See [web-ui/README.md](web-ui/README.md).
