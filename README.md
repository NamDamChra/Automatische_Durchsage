# Automatic Announcement with ESP32

This project enables an automatic announcement using two ESP32 boards. A controller sends a signal via Bluetooth Low Energy (BLE) to the receiver unit, which then plays an audio file through the DFPlayer Mini.

## Contents

- [Features](#features)
- [Hardware](#hardware)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)

## Features

- BLE communication between two ESP32 boards
- Control via rotary encoders
- Automatic time synchronization using NTP
- Audio playback via DFPlayer Mini
- Display output on OLED screen

## Hardware

- 2x ESP32 Dev Kit
- 1x DFPlayer Mini
- 1x Micro SD card
- 1x Speaker
- 1x OLED Display (SSD1306)
- 2x Rotary encoders

## Installation

### 1. Install Required Libraries

Ensure the following libraries are installed in the Arduino IDE:

- `WiFi.h`
- `Wire.h`
- `EEPROM.h`
- `Adafruit_GFX.h`
- `Adafruit_SSD1306.h`
- `DFRobotDFPlayerMini.h`
- `AiEsp32RotaryEncoder.h`
- `BLEDevice.h`

### 2. Wiring

Connect the components as follows:

- **ESP32 Controller:**
  - Rotary encoder: GPIO 41 & 42
  - OLED Display: SDA (GPIO 8), SCL (GPIO 9)
  - WiFi & BLE enabled
- **ESP32 Receiver:**
  - DFPlayer Mini: RX (GPIO 40), TX (GPIO 1)
  - Speaker connected to DFPlayer Mini

### 3. Upload Code to ESP32

- Upload the controller code to the first ESP32
- Upload the receiver code to the second ESP32

## Configuration

### Setting Up WiFi

The ESP32 controller connects to an eduroam network. Adjust the credentials in the `setup()` function:

```cpp
#define EAP_IDENTITY "your_email@domain.com"
#define EAP_USERNAME "your_username"
#define EAP_PASSWORD "your_password"
const char *ssid = "eduroam";
```

### Preparing MP3 Files

Store audio files in the `mp3` folder on the SD card and name them numerically (`0001.mp3`, `0002.mp3`, ...).

## Usage

- Use the rotary encoders to set the alarm time.
- At the scheduled time, the ESP32 sends a BLE signal.
- The receiver plays the audio file via the DFPlayer Mini.
- Optional: Manual triggering via button (GPIO 20).

## Troubleshooting

- **No sound?** Ensure the DFPlayer Mini is wired correctly.
- **BLE connection fails?** Check the controller’s MAC address and update it in the receiver code.
- **Time not updating?** Verify the connection to the NTP server.
