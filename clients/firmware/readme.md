# AnyPrint - Inkplate

This directory contains the code for the Inkplate client.

## Setup

- Install the [Arduino IDE](https://www.arduino.cc/en/software)
- Open this folder in the Arduino IDE
  - (set the sketchbook location to this folder)
  - install the [inkplate board definition](https://github.com/SolderedElectronics/Dasduino-Board-Definitions-for-Arduino-IDE)
  - (install the [drivers](https://soldered.com/learn/ch340-driver-installation-croduino-basic3-nova2/))
  - choose the correct inkplate board in the board menu (6 or 10)

## NOTE

DO NOT UPDATE OR RE-INSTALL THE arduinoWebSockets LIBRARY (currently 2.3.6)!

The version in this repository contains a manual fix (uncommented WebSocketsClient.cpp line 234) for an issue with the Inkplate library.
Without this fix, the websocket connection will not work because `setInsecure()` is not supported by the Inkplate library.
