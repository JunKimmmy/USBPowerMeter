# USB-C Power Meter

A compact USB-C passthrough power meter with real-time voltage, current, and power monitoring displayed on an integrated OLED screen.

## Overview

This board sits inline between a USB-C charger and a device, measuring VBUS voltage and current in real time. All USB signals pass through transparently while the onboard MCU reads the INA238 and drives a 128×64 OLED display.

## Specifications

| Parameter | Value |
|-----------|-------|
| Input voltage range | 0.3 V – 48 V |
| Max continuous current | 5 A |
| Voltage/current resolution | 16-bit (INA238) |
| Display | 128×64 OLED (SSD1315), I2C |
| MCU | STM32G030 |
| Supply (MCU/peripherals) | 3.3 V (onboard buck from VBUS) |
| OLED supply | 3.8 V (onboard boost) |
| Connectivity | USB-C male + female passthrough |

## Hardware

### Key ICs

| IC | Function |
|----|----------|
| INA238 | 16-bit high-precision power/current/voltage monitor |
| STM32G030 | Measurement processing and display control |
| SSD1315 | 128×64 OLED display controller |

### Features

- Full USB-C signal passthrough (power + data lines)
- VBUS-to-3.3 V buck regulator powering MCU and peripherals
- Dedicated 3.8 V boost supply for OLED
- Navigation buttons for menu control
- Power switch
- Buzzer
- SWD debug/programming header

## Repository Structure

```
USBpowermeter.kicad_pro   — KiCad project file
USBpowermeter.kicad_sch   — Schematic
USBpowermeter.kicad_pcb   — PCB layout
libs/
  Library.kicad_sym        — Custom schematic symbols
  Library.pretty/          — Custom footprints
```

## Tools

- KiCad 10

