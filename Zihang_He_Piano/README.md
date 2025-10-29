# SmartRhythm

## My Piano + Metronome MQTT LED Control System

*An individual IoT project for CASA0014: Connected Environments at UCL*

--by Zihang He

------

## Overview

This project combines **music interaction** with **IoT communication**, allowing users to play piano keys and visualize sound and rhythm through a **72-LED luminaire** connected via **MQTT**. Users can switch between practice mode and performance mode to help themselves improve their piano skills.

Here are some links:

[Photos taken during the production process](https://github.com/xms12138/casa0014_Zihang_He/tree/main/Zihang_He_Piano/Images)

[Basic code for connecting to the MQTT server](https://github.com/xms12138/casa0014_Zihang_He/tree/main/Zihang_He_Piano/Arduino_Sketch/2025-10-15_base_connect_code)

[Required hardware and connection methods](https://github.com/xms12138/casa0014_Zihang_He/tree/main/Zihang_He_Piano/Hardware_Components)

[Relevant code related to Vespera](https://github.com/xms12138/casa0014_Zihang_He/tree/main/vespera)

It was developed as part of the **CASA0014: Connected Environments** module at **University College London (UCL)**, which focuses on understanding and building connected IoT systems for people and the environment.

The system uses an **Arduino MKR WiFi 1010**, **Grove LCD display**, **8 mechanical piano keys**, a **slider potentiometer**, and a **state machine switch** to switch between *Performance* and *Metronome* modes.

![1](https://github.com/xms12138/casa0014_Zihang_He/blob/main/Zihang_He_Piano/Images/Final/exterior.jpg)



## Background & Motivation

The idea behind this project was to explore how **physical interaction (music performance)** could be connected to **digital visualization and feedback** through IoT.
 By mapping piano key inputs to LED color patterns and synchronizing rhythm through MQTT, the project demonstrates an **end-to-end IoT pipeline** — from local sensing and actuation to network communication and visualization.

This aligns with CASA0014’s aims to teach prototyping, communication, and environmental connectivity through practical design.



## Features Demonstration

After powering on, the LCD shows *“Ready – Press a key”*.
 In **Performance Mode**, each piano key plays a tone and triggers a unique LED color effect. The LED hue for each key shifts slightly with every press, creating a changing visual rhythm.

Switching to **Metronome Mode**, the slider adjusts the tempo (40–208 BPM). The buzzer ticks at the beat — a higher tone on the first beat — while all LEDs flash in sync with smooth color transitions.

The **LCD display** continuously shows either the current note or the BPM, and its backlight color matches the active LED hue for a synchronized visual effect.



## Hardware Components

| Component                 | Model / Type      | Function                                         |
| ------------------------- | ----------------- | ------------------------------------------------ |
| Arduino MKR WiFi 1010     | ABX00023          | Main controller, WiFi & MQTT                     |
| Grove LCD RGB Backlight   | v2.0              | Displays current note & BPM                      |
| 8 Mechanical Switches     | NO type           | Piano key inputs (C4–C5)                         |
| Slider Potentiometer      | 10kΩ linear       | Adjusts BPM (40–200)                             |
| State Machine Switch      | 2-position toggle | Switch between *Performance* & *Metronome* modes |
| Passive Buzzer            | 5V                | Metronome tick sound                             |
| NeoPixel LED Matrix       | 12×6 (72 LEDs)    | Color and rhythm display                         |
| Breadboard & Jumper Wires | –                 | Wiring and prototyping                           |
| Power Supply              | 5V 2A             | External power for LEDs                          |

For detailed wiring, pin mapping, and diagrams, see [Required hardware and connection methods](https://github.com/xms12138/casa0014_Zihang_He/tree/main/Zihang_He_Piano/Hardware_Components)


------

## Software Structure

| File                      | Function                                                     |
| ------------------------- | ------------------------------------------------------------ |
| `mkr1010_mqtt_simple.ino` | Main sketch – initializes WiFi, MQTT, LED, and input devices |
| `connections.ino`         | Manages WiFi & MQTT reconnection                             |
| `RGBLED.ino`              | Handles NeoPixel color updates                               |
| `arduino_secrets.h`       | Stores WiFi & MQTT credentials (not uploaded for security)   |

#### Key Technology:

**Non-blocking multi-hardware control:**
 All tasks (keys, buzzers, LCD, LEDs, MQTT) run in parallel using `millis()` timing instead of `delay()`.
 Debounced inputs, smoothed analog readings, and automatic Wi-Fi/MQTT reconnection ensure stable performance.

**Two-mode state machine:**
 A physical toggle switch (D10) switches between *Performance* and *Metronome* modes with 25 ms debounce.
 Each mode runs independent logic — piano key effects or BPM-synced rhythm control.

**Dynamic lighting design:**
 Each key maintains its own hue, advancing by 45° per press.
 Procedural effects (Ripple, Comet, Sparkle, Wipe) use additive color blending and exponential fading for smooth visuals.
 In metronome mode, hues shift automatically every beat.

**Accurate tempo engine:**
 The slider input is filtered (median + EMA) and mapped to 40–208 BPM.
 Beats are timestamp-driven, producing precise rhythmic flashes and tones without blocking other functions.

**Custom I²C LCD control:**
 The Grove JHD1313M1 display is driven directly over I²C (0x3E for text, 0x62 for RGB).
 The LCD shows note and BPM info, and its backlight color matches the current hue for synchronized feedback.

------

## MQTT Communication

- **Broker:** `mqtt.cetools.org`

- **Port:** 1884

- **Topic Format:**

  ```
  student/CASA0014/luminaire/<your_ID>
  ```

- **Payload Structure:** RGB byte array for 72 LEDs (216 bytes total).

- **Communication Flow:**

  - Arduino publishes messages when keys are pressed or BPM changes.
  - LED luminaire subscribes and updates colors accordingly.

------

## How to Run

1. Clone this repository:

   ```
   git clone https://github.com/xms12138/casa0014_Zihang_He.git
   ```

2. Open the project in **Arduino IDE**.

3. Create a document called 'arduino_secrets.h' , and write secrets in it.

4. Install required libraries:

   - `WiFiNINA`
   - `PubSubClient`
   - `Adafruit_NeoPixel`
   - `rgb_lcd`

5. Select **Arduino MKR WiFi 1010** as the board.

6. Upload the sketch.

7. Monitor serial output and observe LED + LCD behavior.

------

## Results & Reflection

**Learnings:**

Through this project, I learned how to build and connect circuits with **Arduino**, and how to use **MQTT** to enable real-time communication between devices.
 I also developed a better understanding of **non-blocking programming** and **state-machine design**, which allowed my system to handle multiple hardware tasks simultaneously.
 Most importantly, I realized how **IoT technologies can influence people’s lives** — by turning simple interactions into meaningful, connected experiences that blend the physical and digital worlds.

**Defect:**

At present, the **metronome can only change speed (BPM)** but cannot switch between different **rhythm patterns or time signatures** such as 3/4 or 6/8.
With Arduino, only one buzzer can play a sound at a time. If the metronome and the piano sound overlap, only one of the sounds will be played.

**Inspiration:**

The inspiration came from my own background — I have learned piano for several years, but since coming to the UK, I no longer have access to one. One day, while thinking through another problem, I found myself unconsciously tapping a button rhythmically. That moment gave me the idea: to create a small, interactive piano-metronome device that could reproduce musical rhythm through lights and sound.

------

## Acknowledgments

Developed by **Zihang He** as part of
 **CASA0014: Connected Environments** at **University College London (UCL)**.

Forked and extended from the original UCL CASA repository:
  [ucl-casa-ce/casa0014](https://github.com/ucl-casa-ce/casa0014)

If you have any questions, please contact: zihang.he.24@ucl.ac.uk