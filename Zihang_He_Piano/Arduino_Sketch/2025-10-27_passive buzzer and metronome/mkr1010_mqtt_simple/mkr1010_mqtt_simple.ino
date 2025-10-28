#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"
#include <utility/wifi_drv.h>
#include <math.h>

// ---------- WiFi / MQTT ----------
const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server   = "mqtt.cetools.org";
const int   mqtt_port     = 1884;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

String lightId   = "12";
String mqtt_topic = "student/CASA0014/luminaire/" + lightId;
String clientId   = "";

// 在 connections.ino 中实现
void startWifi();
void reconnectMQTT();
void callback(char* topic, byte* payload, unsigned int length);

// ---------- 72 灯 payload ----------
const int num_leds     = 72;
const int payload_size = num_leds * 3;
byte RGBpayload[payload_size];

// ---------- 8 个按钮 ----------
const uint8_t buttonPins[8] = {1,2,3,4,5,6,7,8}; // 如需更稳可改为 {2,3,4,5,6,7,8,9}
#define USE_NO 1
#if USE_NO
const int ACTIVE_LEVEL = LOW;
#else
const int ACTIVE_LEVEL = HIGH;
#endif
int lastLevel[8];
unsigned long lastDebounceMs[8] = {0};
const unsigned long DEBOUNCE_MS = 25;

// ---------- 每组9灯独立色相 ----------
int   hues[8]  = {0,0,0,0,0,0,0,0};
const int   HUE_STEP = 30;
const float SAT = 1.0f;
const float VAL = 0.9f;

// ---------- 蜂鸣器 ----------
// 钢琴蜂鸣器（按键）：D9
const uint8_t pianoBuzzerPin = 9; // Passive buzzer for piano
bool pianoBuzzActive = false;
unsigned long pianoBuzzOffAt = 0;

// 节拍器蜂鸣器（滴答）：D0  （注意：有些板上 D0/D1 是硬件串口脚）
const uint8_t metroBuzzerPin = 0; // Passive buzzer for metronome
bool metroBuzzActive = false;
unsigned long metroBuzzOffAt = 0;

const uint16_t notesHz[8]   = {262,294,330,349,392,440,494,523}; // C4..C5
const char*    noteNames[8] = {"C4","D4","E4","F4","G4","A4","B4","C5"};

void pianoBeepStart(unsigned freq, unsigned durMs) {
  tone(pianoBuzzerPin, freq);
  pianoBuzzActive = true;
  pianoBuzzOffAt = millis() + durMs;
}
void metroBeepStart(unsigned freq, unsigned durMs) {
  tone(metroBuzzerPin, freq);
  metroBuzzActive = true;
  metroBuzzOffAt = millis() + durMs;
}
void buzzerTask() {
  unsigned long now = millis();
  if (pianoBuzzActive && now >= pianoBuzzOffAt) {
    noTone(pianoBuzzerPin);
    pianoBuzzActive = false;
  }
  if (metroBuzzActive && now >= metroBuzzOffAt) {
    noTone(metroBuzzerPin);
    metroBuzzActive = false;
  }
}

// ---------- Grove LCD ----------
namespace JHD1313M1 {
  const uint8_t I2C_ADDR_TEXT = 0x3E;
  const uint8_t I2C_ADDR_RGB  = 0x62;
  void writeCmd(uint8_t c)   { Wire.beginTransmission(I2C_ADDR_TEXT); Wire.write(0x80); Wire.write(c); Wire.endTransmission(); }
  void writeData(uint8_t d)  { Wire.beginTransmission(I2C_ADDR_TEXT); Wire.write(0x40); Wire.write(d); Wire.endTransmission(); }
  void setRGB(uint8_t r, uint8_t g, uint8_t b) {
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x00); Wire.write(0x00); Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x01); Wire.write(0x00); Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x08); Wire.write(0xAA); Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x04); Wire.write(r); Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x03); Wire.write(g); Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x02); Wire.write(b);
  }
  void init() {
    Wire.begin();
    delay(50);
    writeCmd(0x38); writeCmd(0x39); writeCmd(0x14); writeCmd(0x70);
    writeCmd(0x56); writeCmd(0x6C); delay(200);
    writeCmd(0x38); writeCmd(0x0C); writeCmd(0x01); delay(2);
    setRGB(0, 128, 255);
  }
  void clear() { writeCmd(0x01); delay(2); }
  void setCursor(uint8_t col, uint8_t row) {
    static const uint8_t row_addr[2] = {0x80, 0xC0};
    writeCmd(row_addr[row] + col);
  }
  void printLine(uint8_t row, const char* text) {
    setCursor(0, row);
    for (uint8_t i = 0; i < 16; ++i) {
      char c = text[i];
      if (!c) { for (; i < 16; ++i) writeData(' '); break; }
      writeData(c);
    }
  }
}
// 修正上面 setRGB 小笔误（Write->write）
namespace JHD1313M1 { void setRGB_fix(){} } // 占位避免重复定义

// ---------- 工具函数 ----------
void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
  float c = v * s;
  float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
  float m = v - c;
  float r1,g1,b1;
  if      (h < 60)  { r1=c; g1=x; b1=0; }
  else if (h <120 ) { r1=x; g1=c; b1=0; }
  else if (h <180 ) { r1=0; g1=c; b1=x; }
  else if (h <240 ) { r1=0; g1=x; b1=c; }
  else if (h <300 ) { r1=x; g1=0; b1=c; }
  else              { r1=c; g1=0; b1=x; }
  r=(uint8_t)((r1+m)*255); g=(uint8_t)((g1+m)*255); b=(uint8_t)((b1+m)*255);
}

void send_all_off() {
  if (!mqttClient.connected()) return;
  memset(RGBpayload, 0, payload_size);
  mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
}
void set_only_block_rgb(uint8_t groupIdx, uint8_t r, uint8_t g, uint8_t b) {
  if (!mqttClient.connected()) return;
  memset(RGBpayload, 0, payload_size);
  int startPixel = groupIdx * 9;
  int endPixel   = startPixel + 8;
  for (int p = startPixel; p <= endPixel; ++p) {
    RGBpayload[p*3+0] = r;
    RGBpayload[p*3+1] = g;
    RGBpayload[p*3+2] = b;
  }
  mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
}
void set_all_rgb(uint8_t r, uint8_t g, uint8_t b) {
  if (!mqttClient.connected()) return;
  for (int p = 0; p < num_leds; ++p) {
    RGBpayload[p*3+0] = r;
    RGBpayload[p*3+1] = g;
    RGBpayload[p*3+2] = b;
  }
  mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
}

// ---------- 模式与滑动电位器 ----------
const uint8_t modeSwitchPin = 10;  // 拨到 I=LOW, 拨到 O=HIGH
const int potPin = A0;             // 滑动电位器

int lastModeLevel = HIGH;
unsigned long lastModeDebounce = 0;
const unsigned long MODE_DEBOUNCE_MS = 25;

int potValue = 0;
int bpm = 60;
unsigned long beatInterval = 1000;
unsigned long nextBeatAt = 0;
int hue_auto = 0;
uint8_t beatInBar = 0; // 4/4 拍内计数

// === 新增：稳定读取 BPM（5次采样中值 + EMA + 死区） ===
int readStableBpm() {
  // (a) 多次采样取中值
  const uint8_t N = 5;
  uint16_t s[N];
  for (uint8_t i=0;i<N;i++) { s[i] = analogRead(potPin); }
  for (uint8_t i=0;i<N;i++) for (uint8_t j=i+1;j<N;j++) if (s[j]<s[i]) { uint16_t t=s[i]; s[i]=s[j]; s[j]=t; }
  uint16_t raw = s[N/2];

  // (b) EMA 滤波
  static float ema = -1;
  if (ema < 0) ema = raw;
  const float alpha = 0.20f;             // 越小越稳：0.1~0.3
  ema = alpha*raw + (1.0f-alpha)*ema;

  // (c) 0..4095 → 40..208
  int bpmNew = map((int)ema, 0, 4095, 40, 208);

  // (d) 死区（±2 BPM 内不更新）
  static int bpmShown = 120;
  const int deadband = 2;
  if (abs(bpmNew - bpmShown) >= deadband) bpmShown = bpmNew;

  return bpmShown;
}

// ---------- setup ----------
void setup() {
  Serial.begin(115200);

  for (int i=0;i<8;++i) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastLevel[i] = digitalRead(buttonPins[i]);
  }

  pinMode(pianoBuzzerPin, OUTPUT);
  pinMode(metroBuzzerPin, OUTPUT);

  pinMode(modeSwitchPin, INPUT_PULLUP);
  pinMode(potPin, INPUT);

  // 关键：设置为 12-bit 分辨率（0..4095）
  analogReadResolution(12);
  analogReference(AR_DEFAULT); // 3.3V 参考

  JHD1313M1::init();
  JHD1313M1::printLine(0, "Ready");
  JHD1313M1::printLine(1, "Press a key");

  startWifi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(2000);
  mqttClient.setCallback(callback);

  send_all_off();
}

// ---------- loop ----------
void loop() {
  if (!mqttClient.connected()) reconnectMQTT();
  if (WiFi.status() != WL_CONNECTED) startWifi();
  mqttClient.loop();

  unsigned long now = millis();

  // === 模式检测 ===
  int modeLevel = digitalRead(modeSwitchPin);
  if (modeLevel != lastModeLevel && (now - lastModeDebounce) > MODE_DEBOUNCE_MS) {
    lastModeDebounce = now;
    lastModeLevel = modeLevel;
    if (modeLevel == LOW) { nextBeatAt = 0; beatInBar = 0; } // I 模式重新起拍
  }

  // === 钢琴系统（总是工作：蜂鸣 + LCD；O 模式才改灯） ===
  for (uint8_t i = 0; i < 8; ++i) {
    int level = digitalRead(buttonPins[i]);
    if (level != lastLevel[i] && (now - lastDebounceMs[i]) > DEBOUNCE_MS) {
      lastDebounceMs[i] = now;

      if (level == ACTIVE_LEVEL && lastLevel[i] != ACTIVE_LEVEL) {
        // 钢琴蜂鸣（D9）
        pianoBeepStart(notesHz[i], 120);

        // LCD
        char line[17];
        snprintf(line, sizeof(line), "Note: %-3s", noteNames[i]);
        JHD1313M1::printLine(0, line);

        uint8_t r,g,b;
        hsvToRgb((float)hues[i], SAT, VAL, r, g, b);
        JHD1313M1::setRGB(r/2, g/2, b/2);

        // 只有在 O 模式下才改 72 灯
        if (modeLevel == HIGH) {
          set_only_block_rgb(i, r, g, b);
          hues[i] = (hues[i] + HUE_STEP) % 360;
        }
      }
      lastLevel[i] = level;
    }
  }

  // === I 模式：滑动电位器控制节拍器（灯 + 蜂鸣器D0） ===
  if (modeLevel == LOW) { // 拨到 I
    bpm = readStableBpm();                  // <—— 使用稳定 BPM
    beatInterval = 60000UL / bpm;          // 每拍间隔 ms

    if (now >= nextBeatAt) {
      nextBeatAt = now + beatInterval;

      // 1) 节拍器蜂鸣（D0）：强拍更高/更长、弱拍稍低/短些
      if (beatInBar == 0) metroBeepStart(1600, 55);  // 强拍
      else                metroBeepStart(1200, 40);  // 弱拍

      // 2) 72 灯：每拍推进色相 + 整条更新
      hue_auto = (hue_auto + 30) % 360;
      uint8_t r,g,b;
      hsvToRgb((float)hue_auto, 1.0, 0.9, r, g, b);
      set_all_rgb(r, g, b);

      // 3) 小节计数（4/4）
      beatInBar = (beatInBar + 1) % 4;

      // 4) LCD 显示当前 BPM
      char bpmLine[17];
      snprintf(bpmLine, sizeof(bpmLine), "BPM:%3d", bpm);
      JHD1313M1::printLine(1, bpmLine);
    }
  }

  // 关蜂鸣器（两路）
  buzzerTask();
}
