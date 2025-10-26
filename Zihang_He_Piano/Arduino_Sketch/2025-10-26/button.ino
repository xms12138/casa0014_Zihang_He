// 8-key piano with mechanical switches (COM->GND, NO->Dx), MKR WiFi 1010
// Keys on D1..D8, passive buzzer on D9
// Wiring for each key: COM -> GND, NO -> Dx (no VCC needed)

#include <Arduino.h>

const uint8_t buzzerPin = 9;
const uint8_t buttonPins[8] = {1, 2, 3, 4, 5, 6, 7, 8};   // D1..D8

// C major scale: C4..C5 (do..high do)
const uint16_t notesHz[8] = {262, 294, 330, 349, 392, 440, 494, 523};

const uint16_t DEBOUNCE_MS = 25;   // 防抖时间
int currentNote = -1;

// 记录每个键的上次稳定状态和时间（主动上拉，按下=LOW）
bool lastStablePressed[8] = {false};
uint32_t lastChangeMs[8]   = {0};

void setup() {
  // 机械开关：使用内部上拉，空闲=HIGH，按下=LOW
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastStablePressed[i] = false;
    lastChangeMs[i] = 0;
  }
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  int pressedIndex = -1;

  // 扫描 8 个键，带简单防抖；优先取编号小的那个键（单音）
  for (uint8_t i = 0; i < 8; i++) {
    bool rawPressed = (digitalRead(buttonPins[i]) == LOW);  // 按下=LOW
    uint32_t now = millis();

    if (rawPressed != lastStablePressed[i]) {
      // 状态发生变化，启动/更新防抖计时
      if (now - lastChangeMs[i] >= DEBOUNCE_MS) {
        lastStablePressed[i] = rawPressed;       // 变化超过防抖时间，确认新的稳定状态
        lastChangeMs[i] = now;
      }
    }

    if (lastStablePressed[i]) {  // 找到第一个被按下的键
      pressedIndex = i;
      break;
    }
  }

  if (pressedIndex != -1) {
    if (pressedIndex != currentNote) {
      tone(buzzerPin, notesHz[pressedIndex]);
      currentNote = pressedIndex;
    }
  } else if (currentNote != -1) {
    noTone(buzzerPin);
    currentNote = -1;
  }

  delay(2); // 轻微让步 CPU
}
