# Smart Door Lock System using STM32F401RE + RFID RC522

## Overview

This project is a smart door lock system developed using **STM32F401RE** with bare-metal register programming.

The system supports:

- Password authentication using a 4x4 keypad
- RFID authentication using RC522
- Servo motor door control
- Password storage in internal Flash memory
- LCD I2C user interface
- Alarm and security lockout protection

The project is implemented without HAL libraries to improve low-level embedded programming skills and hardware understanding.

---

# Features

## Password Authentication

- 6-digit password input
- Backspace support using `#`
- Change password mode using `*`

## RFID Authentication

- RC522 RFID reader via SPI1
- Authorized UID verification
- Automatic door unlock for valid RFID card

## Security Protection

- Wrong password counter
- Buzzer alarm on incorrect password
- System lock for 30 seconds after 5 failed attempts

## Flash Memory Storage

- Password stored in STM32 internal Flash
- Password retained after power reset

## Door Control

- Servo motor control using PWM (TIM3)
- LED status indication

---

# Hardware Used

| Component     | Description            |
| ------------- | ---------------------- |
| STM32F401RE   | Main MCU               |
| RC522         | RFID Reader            |
| SG90 Servo    | Door lock actuator     |
| 16x2 LCD I2C  | Display                |
| 4x4 Keypad    | Password input         |
| Buzzer        | Alarm                  |
| LED           | Door status indication |

---

# Peripheral Configuration

## Timer

- TIM2 → Delay function
- TIM3 CH3 → Servo PWM

## Communication

- SPI1 → RC522 RFID
- I2C1 → LCD I2C

## GPIO

- PC8 → Door status LED
- PC7 → Buzzer

---

# RFID Authentication

Authorized UID:

```c
static const uint8_t authorized_uid[4] =
{0xF9, 0x3F, 0x4D, 0x06};
```

Authentication workflow:

1. Detect RFID card
2. Read UID
3. Compare with authorized UID
4. Unlock door if matched

---

# Flash Memory Storage

Password storage address:

```c
#define FLASH_PASS_ADDR 0x08020000
```

Implemented operations:

- Flash unlock
- Sector erase
- Password write
- Persistent storage after reset

---

# Project Structure

```text
Core Modules:
│
├── Timer Module
├── Servo PWM Module
├── Keypad Module
├── LCD I2C Module
├── Flash Storage Module
├── RFID RC522 Module
└── Smart Lock FSM
```

---

# Demo Workflow

## Unlock using Password

1. Enter 6-digit password
2. System verifies password
3. Servo opens door
4. Door closes automatically after timeout

## Unlock using RFID

1. Scan RFID card
2. System checks UID
3. Door unlocks if authorized

---

# Development Environment

- STM32CubeIDE
- STM32F401CCU6
- Embedded C
- CMSIS Register-Level Programming

---

# Future Improvements

- Multiple RFID card support
- External EEPROM storage
- Bluetooth/WiFi remote unlock
- Mobile application integration
- OLED display UI
- FreeRTOS integration

---

# How to Build

1. Open project in STM32CubeIDE
2. Build project
3. Flash firmware using ST-Link
4. Connect peripherals
5. Power on system

---

# Pin Configuration

| Peripheral | STM32 Pin |
| ---------- | --------- |
| RC522 SCK  | PB3       |
| RC522 MISO | PB4       |
| RC522 MOSI | PB5       |
| RC522 CS   | PB12      |
| RC522 RST  | PB2       |
| Servo PWM  | PB0       |
| LCD SDA    | PB9       |
| LCD SCL    | PB8       |
| Buzzer     | PC7       |
| LED        | PC8       |

---

# Author

**Đức Anh Nguyễn - 23020780**

**Nguyễn Quang Bảo - 23020784**

Introduction to Embedded Systems — UET - VNU
