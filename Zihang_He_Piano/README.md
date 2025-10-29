# Piano + Metronome MQTT LED Control System

*An individual IoT project for CASA0014: Connected Environments at UCL*

------

## Overview

This project combines **music interaction** with **IoT communication**, allowing users to play piano keys and visualize sound and rhythm through a **72-LED luminaire** connected via **MQTT**. Users can switch between practice mode and performance mode to help themselves improve their piano skills.

It was developed as part of the **CASA0014: Connected Environments** module at **University College London (UCL)**, which focuses on understanding and building connected IoT systems for people and the environment.

The system uses an **Arduino MKR WiFi 1010**, **Grove LCD display**, **8 mechanical piano keys**, a **slider potentiometer**, and a **state machine switch** to switch between *Performance* and *Metronome* modes.

![1](https://github.com/xms12138/casa0014_Zihang_He/blob/main/Zihang_He_Piano/Images/Final/exterior.jpg)

------

## 🎓 Background & Motivation

The idea behind this project was to explore how **physical interaction (music performance)** could be connected to **digital visualization and feedback** through IoT.
 By mapping piano key inputs to LED color patterns and synchronizing rhythm through MQTT, the project demonstrates an **end-to-end IoT pipeline** — from local sensing and actuation to network communication and visualization.

This aligns with CASA0014’s aims to teach prototyping, communication, and environmental connectivity through practical design.

------

## ⚙️ System Architecture

```
+---------------------------+
| Piano Keys (8 switches)   |
| Slider (BPM control)      |
| Mode Switch (2 modes)     |
+-------------+-------------+
              |
              v
+----------------------------------+
| Arduino MKR WiFi 1010            |
|  - Reads piano keys & slider     |
|  - Controls LCD & buzzer         |
|  - Publishes data via MQTT       |
+----------------------------------+
              |
              v
+----------------------------------+
| MQTT Broker (mqtt.cetools.org)   |
| Topic: student/CASA0014/luminaire/<ID> |
+----------------------------------+
              |
              v
+----------------------------------+
| LED Luminaire (72 NeoPixels)     |
| Visual feedback of notes & BPM   |
+----------------------------------+
```

------

## 🧱 Hardware Components

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

📁 For detailed wiring, pin mapping, and diagrams, see
 👉 `/hardware_components/README.md`

------

## 💻 Software Structure

| File                      | Function                                                     |
| ------------------------- | ------------------------------------------------------------ |
| `mkr1010_mqtt_simple.ino` | Main sketch – initializes WiFi, MQTT, LED, and input devices |
| `connections.ino`         | Manages WiFi & MQTT reconnection                             |
| `RGBLED.ino`              | Handles NeoPixel color updates                               |
| `arduino_secrets.h`       | Stores WiFi & MQTT credentials (not uploaded for security)   |

**Core Functions:**

1. **WiFi + MQTT Setup:** Connects to `mqtt.cetools.org` on port `1884`.
2. **State Machine:** Switches between *Performance* and *Metronome* modes via D10 input.
3. **Piano Input:** Reads digital pins D1–D8 for key presses.
4. **BPM Control:** Uses slider on A0 to map analog values to BPM.
5. **Buzzer:** Generates rhythmic ticks or note feedback.
6. **LCD:** Displays `Note: C4` and `BPM: 120` dynamically.
7. **LED Output:** Publishes RGB payloads via MQTT to control a 72-LED luminaire.

------

## 🌐 MQTT Communication

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

## 🎹 Features Demonstration

| Mode                 | Description                                                  | Visual / Audio Feedback             |
| -------------------- | ------------------------------------------------------------ | ----------------------------------- |
| **Performance Mode** | Each piano key triggers a unique LED color pattern and tone. | Instant LED color change and sound. |
| **Metronome Mode**   | LEDs flash at BPM speed while buzzer ticks rhythmically.     | Adjustable tempo with slider.       |
| **LCD Display**      | Shows current note and BPM in real-time.                     | Dynamic screen updates.             |

📸 *You can include demonstration photos or videos here.*

------

## 🧪 How to Run

1. Clone this repository:

   ```
   git clone https://github.com/xms12138/casa0014_Zihang_He.git
   ```

2. Open the project in **Arduino IDE**.

3. Install required libraries:

   - `WiFiNINA`
   - `PubSubClient`
   - `Adafruit_NeoPixel`
   - `rgb_lcd`

4. Edit `/arduino_secrets.h` with your WiFi and MQTT credentials.

5. Select **Arduino MKR WiFi 1010** as the board.

6. Upload the sketch.

7. Monitor serial output and observe LED + LCD behavior.

------

## ⚙️ Technical Details

- **State Machine Logic:**

  ```
  int modeSwitch = digitalRead(10);
  if (modeSwitch == LOW) {
    currentMode = PERFORMANCE_MODE;
  } else {
    currentMode = METRONOME_MODE;
  }
  ```

- **BPM Mapping:**

  ```
  bpm = map(analogRead(A0), 0, 1023, 40, 200);
  ```

- **Non-blocking timing** implemented with `millis()` for LED and buzzer synchronization.

- **Debouncing** for piano keys ensures accurate note triggering.

- **Efficient MQTT** messages to minimize latency and bandwidth.

------

## 📊 Results & Reflection

✅ **Achievements:**

- Successful real-time connection between physical input and MQTT visual output.
- Smooth mode switching and BPM synchronization.
- Stable WiFi and MQTT performance with minimal lag.

💡 **Learnings:**

- Implementing a functional state machine to control complex IoT behaviors.
- Managing timing, LED animation, and communication efficiently.
- Understanding how physical interaction can drive connected environment systems.

🚀 **Future Improvements:**

- Add web dashboard visualization for remote control.
- Store data logs (note frequency, BPM history).
- Implement sound frequency-based LED animations.

------

## 🙌 Acknowledgments

Developed by **Zihang He** as part of
 **CASA0014: Connected Environments** at **University College London (UCL)**.

Forked and extended from the original UCL CASA repository:
 👉 [ucl-casa-ce/casa0014](https://github.com/ucl-casa-ce/casa0014)