

# Hardware Components Overview

This folder documents all hardware components used in the **Piano + Metronome MQTT LED Control System**.  
The system combines manual piano key input, metronome control, and real-time visual feedback via a 72-LED MQTT-connected luminaire.

---

##  1. Arduino MKR WiFi 1010

###  Description
- **Model:** Arduino MKR WiFi 1010  
- **Functions:**
  - Acts as the **main microcontroller**.
  - Handles **WiFi connection** and **MQTT communication** with the broker `mqtt.cetools.org`.
  - Reads sensor and switch inputs.
  - Controls LEDs and buzzer output.

### Wiring
| Pin       | Connection              | Description                                                |
| --------- | ----------------------- | ---------------------------------------------------------- |
| D1–D8     | Mechanical switches     | Piano key inputs                                           |
| D0/D9     | Buzzer output           | Produces metronome tick /sound of a piano                  |
| D10       | State machine switch    | Used for switching between playing mode and metronome mode |
| A0        | Slider potentiometer    | Reads BPM value                                            |
| SDA / SCL | Grove LCD I²C interface | Display data output                                        |
| 5V / GND  | Breadboard power rails  | Power supply for all components                            |

### Notes
- Connect GND to all components for a shared ground reference.  
- Use a 5V 2A power source when LEDs are connected to avoid current drops.

---



## 2. Grove LCD RGB Backlight Display

### Description
- **Model:** Grove LCD RGB Backlight v2.0  
- **Library:** `rgb_lcd.h`
- **Functions:**
  - Displays **current note (e.g., “C4”, “F4”)**.
  - Displays **current BPM (beats per minute)** of the metronome.

### Wiring
| Grove Pin | Arduino Pin | Description |
| --------- | ----------- | ----------- |
| GND       | GND         | Ground      |
| VCC       | 5V          | Power       |
| SDA       | SDA         | I²C data    |
| SCL       | SCL         | I²C clock   |



## 3. Piano Key Switches (8 Mechanical Buttons)

### Description

- **Type:** NO (Normally Open) mechanical switches.
- **Quantity:** 8
- **Function:** Simulate piano keys (C4–C5 notes range).

### Wiring

| Component | Arduino Pin | Description |
| --------- | ----------- | ----------- |
| Key 1     | D1          | Note C4     |
| Key 2     | D2          | Note D4     |
| Key 3     | D3          | Note E4     |
| Key 4     | D4          | Note F4     |
| Key 5     | D5          | Note G4     |
| Key 6     | D6          | Note A4     |
| Key 7     | D7          | Note B4     |
| Key 8     | D8          | Note C5     |

Each switch connects:

- One terminal → Arduino digital pin
- Another terminal → GND

Use `pinMode(pin, INPUT_PULLUP)` to keep stable input (LOW when pressed).



## 4. Slider Switch (BPM Controller)

### Description

- **Type:** Linear potentiometer (10kΩ)
- **Function:** Adjusts the **metronome speed (BPM)**.

### Wiring

| Potentiometer Pin   | Arduino Pin | Description                |
| ------------------- | ----------- | -------------------------- |
| GND                 | GND         | Ground                     |
| VCC                 | 3.3V        | Power                      |
| Signal (middle pin) | A0          | Analog input for BPM value |

Use these to change BPM.



## 5. Passive Buzzer

### Description

- **Type:** 5V passive buzzer.
- **Function:** Produces sound pulses at each beat of the metronome.

### Wiring

| Buzzer Pin | Arduino Pin | Description |
| ---------- | ----------- | ----------- |
| Positive   | D0/D9       | PWM output  |
| Negative   | GND         | Ground      |



## 6. State Machine Switch (Mode Selector)

**Type:** 2-position toggle
 **Function:**
 This switch is used to **toggle between two modes**:

- **Performance Mode:** Piano keys trigger note sounds and send corresponding LED color patterns through MQTT.
- **Metronome Mode:** The system acts as a metronome, producing rhythmic ticks and synchronized LED flashing based on BPM.

### Wiring

Using an **SPST (two-pin)** switch:

| Switch Pin     | Arduino Pin | Description                    |
| -------------- | ----------- | ------------------------------ |
| One terminal   | GND         | Ground                         |
| Other terminal | D10         | Digital input for mode control |