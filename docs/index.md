# eiBeacon

An iBeacon in the shape of an egg.

## Parts

- ESP32C3 Super Mini Module
- 1x Cr123A Battery Holder
- 1x Cr123A Battery
- 1x switch
- 2x ws2812b 5050 RGB LED
- 1x 3D Printed Egg Top
- 1x 3D Printed Egg Base

## PCB

The PCB and schematic were drawn in [EasyEDA](https://oshwlab.com/bpijls/crowdcontroller_copy). It's a 2 layer board designed to fit in the egg shell.

|peripheral| pin|
|--|--|
|button| GPIO 9|
|WS2812 LED| GPIO 5|

|![eiBeaconSchematic](./schematic_eiBeacon.png)|
|-|
|eiBeacon Schematic|

|![eiBeaconPCBTop](eiBeaconPCBTop.png)|![eiBeaconPCBBottom](eiBeaconPCBBottom.png)|
|-|-|
|eiBeacon PCB top|eiBeacon PCB Bottom|

## Firmware

The [firmware](https://github.com/bpijls/ei-beacon/tree/main/ei-beacon) is based on the iBeacon example in the ESP32 Arduino core.

- When the `boot` pin is pressed after reset, the device will enter a mode where it is possible to enter the beacon name and the beacon UUID on the serial port.
- The device flashes it's LED every 60 seconds. It cycles through 6 colors.

## Enclosure

The enclosure is drawn in rhino and can be printed in 2 parts.

!!! note "Print with support"

    Print both parts with their opening on the print-bed. Support is needed for the bottom of both parts. Support for the inside of the shell can be turned off.

|![](shell-parts.png)|![](shell-pcb-slot.png)|
|-|-|
|egg shell parts|egg shell PCB slot|

A slot is cut into both shells in order to keep the pcb in place.

!!! note "PCB slot"

    Pleas align the slots when closing the shell so the PCB is kept in place.
