// 7-key piano with all Grove buttons (press = HIGH), MKR WiFi 1010 safe
// Buttons on D2..D8, passive buzzer on D9
// Wire each Grove: GND->GND, VCC->3V3, SIG->Dx

const uint8_t buzzerPin = 9;
const uint8_t buttonPins[7] = {1, 2, 3, 4, 5, 6, 7};

// C major (C4..B4)
const uint16_t notesHz[7] = {262, 294, 330, 349, 392, 440, 494};

int currentNote = -1;

void setup() {
  for (uint8_t i = 0; i < 7; i++) {
    pinMode(buttonPins[i], INPUT_PULLDOWN);  // 用下拉电阻稳定信号
  }
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  int pressedIndex = -1;

  for (uint8_t i = 0; i < 7; i++) {
    if (digitalRead(buttonPins[i]) == HIGH) {  // 按下=HIGH
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

  delay(10);
}
