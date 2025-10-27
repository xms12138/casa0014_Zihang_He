// MKR WiFi 1010 + 8键 + passive 蜂鸣器 + Grove LCD (JHD1313M1)
// D1..D8：机械开关（COM->GND, NO->Dx）
// D9：蜂鸣器
// I2C LCD：0x3E(文本) + 0x62(RGB 背光)

#include <Arduino.h>
#include <Wire.h>

// ---------- 钢琴部分 ----------
const uint8_t buzzerPin = 9;
const uint8_t buttonPins[8] = {1,2,3,4,5,6,7,8};   // D1..D8
const uint16_t notesHz[8]   = {262,294,330,349,392,440,494,523}; // C4..C5
const char*    noteNames[8] = {"C4","D4","E4","F4","G4","A4","B4","C5"};

const uint16_t DEBOUNCE_MS = 25;
int currentNote = -1;  // 当前正在发声的键索引（-1 表示无）

bool lastStablePressed[8] = {false};
uint32_t lastChangeMs[8]   = {0};

// ---------- LCD 部分 ----------
#define LCD_ADDR 0x3E
#define RGB_ADDR 0x62

bool lcdInitOnce(uint8_t ch=0x70, uint8_t cl=0x56) {
  Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(0x38); Wire.endTransmission(); delay(10);
  Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(0x39); Wire.endTransmission(); delay(10);
  Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(0x14); Wire.endTransmission();
  Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(ch);   Wire.endTransmission();
  Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(cl);   Wire.endTransmission();
  Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(0x6C); Wire.endTransmission(); delay(200);
  Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(0x38); Wire.endTransmission();
  Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(0x0C); Wire.endTransmission();
  Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(0x01); Wire.endTransmission(); delay(2);
  Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(0x06); Wire.endTransmission();
  return true;
}

void setRGB(uint8_t r,uint8_t g,uint8_t b){
  Wire.beginTransmission(RGB_ADDR); Wire.write(0x00); Wire.write(0x00); Wire.endTransmission();
  Wire.beginTransmission(RGB_ADDR); Wire.write(0x08); Wire.write(0xAA); Wire.endTransmission();
  Wire.beginTransmission(RGB_ADDR); Wire.write(0x04); Wire.write(r);    Wire.endTransmission();
  Wire.beginTransmission(RGB_ADDR); Wire.write(0x03); Wire.write(g);    Wire.endTransmission();
  Wire.beginTransmission(RGB_ADDR); Wire.write(0x02); Wire.write(b);    Wire.endTransmission();
}

void lcdCmd(uint8_t c){ Wire.beginTransmission(LCD_ADDR); Wire.write(0x80); Wire.write(c); Wire.endTransmission(); }
void lcdData(uint8_t d){ Wire.beginTransmission(LCD_ADDR); Wire.write(0x40); Wire.write(d); Wire.endTransmission(); }

void lcdPrintLine(uint8_t line, const char* s){
  uint8_t addr = (line==0) ? 0x80 : 0xC0;
  lcdCmd(addr);
  char ch;
  uint8_t i=0;
  for(; (ch=s[i]) && i<16; ++i) lcdData(ch);
  for(; i<16; ++i) lcdData(' ');
}

// ---------- 初始化 ----------
void setup() {
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastStablePressed[i] = false;
    lastChangeMs[i] = 0;
  }
  pinMode(buzzerPin, OUTPUT);

  delay(600);
  Wire.begin();
  Wire.setClock(100000);
  lcdInitOnce();
  setRGB(0,128,255);

  lcdPrintLine(0, "Note: --");         // 开机默认显示
  lcdPrintLine(1, "hello, world");
}

// ---------- 主循环 ----------
void loop() {
  int pressedIndex = -1;
  uint32_t now = millis();

  // 防抖扫描
  for (uint8_t i = 0; i < 8; i++) {
    bool rawPressed = (digitalRead(buttonPins[i]) == LOW);
    if (rawPressed != lastStablePressed[i]) {
      if (now - lastChangeMs[i] >= DEBOUNCE_MS) {
        lastStablePressed[i] = rawPressed;
        lastChangeMs[i] = now;
      }
    }
    if (lastStablePressed[i]) { pressedIndex = i; break; }
  }

  if (pressedIndex != -1) {
    // 有键被按下：如果换了音，就发声并更新 LCD 第一行
    if (pressedIndex != currentNote) {
      tone(buzzerPin, notesHz[pressedIndex]);
      currentNote = pressedIndex;

      char line[17];
      snprintf(line, sizeof(line), "Note: %-3s     ", noteNames[pressedIndex]);
      lcdPrintLine(0, line);           // 只在“按下新键”时更新显示
    }
  } else {
    // 松手：停止发声，但不改变 LCD 显示（保留上一次的音名）
    if (currentNote != -1) {
      noTone(buzzerPin);
      currentNote = -1;
      // 不更新 lcdPrintLine(0, "Note: --");
    }
  }

  delay(2);
}
