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

// 在 connections.ino 中实现（保持你现有的）
void startWifi();
void reconnectMQTT();
void callback(char* topic, byte* payload, unsigned int length);

// ---------- 72 灯 payload ----------
const int num_leds     = 72;
const int payload_size = num_leds * 3;
byte RGBpayload[payload_size];

// ---------- 8 个按钮（NO→Dx，COM→GND） ----------
const uint8_t buttonPins[8] = {1,2,3,4,5,6,7,8}; // 如遇 D1 不稳，可用 {2,3,4,5,6,7,8,9}
#define USE_NO 1
#if USE_NO
const int ACTIVE_LEVEL = LOW;   // INPUT_PULLUP: 未按=HIGH, 按下=LOW
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
const uint8_t buzzerPin = 9; // Passive
const uint16_t notesHz[8]   = {262,294,330,349,392,440,494,523}; // C4..C5
const char*    noteNames[8] = {"C4","D4","E4","F4","G4","A4","B4","C5"};
bool buzzerActive = false;
unsigned long buzzerOffAt = 0;
void buzzerStart(unsigned freq, unsigned durMs) {
  tone(buzzerPin, freq);
  buzzerActive = true;
  buzzerOffAt = millis() + durMs;
}
void buzzerTask() {
  if (buzzerActive && millis() >= buzzerOffAt) {
    noTone(buzzerPin);
    buzzerActive = false;
  }
}

// ---------- Grove LCD JHD1313M1 最小驱动 ----------
namespace JHD1313M1 {
  const uint8_t I2C_ADDR_TEXT = 0x3E; // 文本控制器
  const uint8_t I2C_ADDR_RGB  = 0x62; // 背光控制器
  void writeCmd(uint8_t c)   { Wire.beginTransmission(I2C_ADDR_TEXT); Wire.write(0x80); Wire.write(c); Wire.endTransmission(); }
  void writeData(uint8_t d)  { Wire.beginTransmission(I2C_ADDR_TEXT); Wire.write(0x40); Wire.write(d); Wire.endTransmission(); }
  void setRGB(uint8_t r, uint8_t g, uint8_t b) {
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x00); Wire.write(0x00); Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x01); Wire.write(0x00); Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x08); Wire.write(0xAA); Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x04); Wire.write(r); Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x03); Wire.write(g); Wire.endTransmission();
    Wire.beginTransmission(I2C_ADDR_RGB); Wire.write(0x02); Wire.write(b); Wire.endTransmission();
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
} // namespace

// ---------- 工具函数 ----------
void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i > 0) Serial.print(":");
  }
  Serial.println();
}

void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
  float c = v * s;
  float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
  float m = v - c;
  float r1, g1, b1;
  if      (h < 60)  { r1 = c; g1 = x; b1 = 0; }
  else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
  else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
  else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
  else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
  else              { r1 = c; g1 = 0; b1 = x; }
  r = (uint8_t)((r1 + m) * 255.0f);
  g = (uint8_t)((g1 + m) * 255.0f);
  b = (uint8_t)((b1 + m) * 255.0f);
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

// ===== 新增：整条同色 =====
void set_all_rgb(uint8_t r, uint8_t g, uint8_t b) {
  if (!mqttClient.connected()) return;
  for (int p = 0; p < num_leds; ++p) {
    RGBpayload[p*3+0] = r;
    RGBpayload[p*3+1] = g;
    RGBpayload[p*3+2] = b;
  }
  mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
}

// ===== 模式开关（D10；I=LOW 自动，O=HIGH 按键）=====
const uint8_t modeSwitchPin = 10;       // 一脚接 GND，一脚接 D10
int lastModeLevel = HIGH;               // 上次读值
unsigned long lastModeDebounce = 0;
const unsigned long MODE_DEBOUNCE_MS = 25;

// 自动模式计时
unsigned long lastAutoMs = 0;
const unsigned long AUTO_INTERVAL_MS = 1000;
int autoHue = 0;

// ---------- setup / loop ----------
void setup() {
  Serial.begin(115200);

  byte mac[6]; WiFi.macAddress(mac);
  Serial.print("MAC: "); printMacAddress(mac);
  Serial.print("Vespera "); Serial.println(lightId);

  for (int i=0;i<8;++i) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastLevel[i] = digitalRead(buttonPins[i]);
  }
  pinMode(buzzerPin, OUTPUT);

  // 模式开关
  pinMode(modeSwitchPin, INPUT_PULLUP);
  lastModeLevel = digitalRead(modeSwitchPin);

  JHD1313M1::init();
  JHD1313M1::printLine(0, "Ready          ");
  JHD1313M1::printLine(1, "Press a key    ");

  startWifi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(2000);
  mqttClient.setCallback(callback);

  send_all_off();
}

void loop() {
  // 网络保活
  if (!mqttClient.connected()) reconnectMQTT();
  if (WiFi.status() != WL_CONNECTED) startWifi();
  mqttClient.loop();

  unsigned long now = millis();

  // ==== 读取并消抖模式开关 ====
  int modeLevel = digitalRead(modeSwitchPin);
  if (modeLevel != lastModeLevel && (now - lastModeDebounce) > MODE_DEBOUNCE_MS) {
    lastModeDebounce = now;
    lastModeLevel = modeLevel;

    // 切模式时，重置一下自动计时 / 或清灯
    if (modeLevel == LOW) {           // I = 自动
      lastAutoMs = 0;                 // 立即执行一次自动变色
    } else {                          // O = 按键
      send_all_off();                 // 回到按键模式先清一遍
    }
  }

  // ==== 扫描 8 个键：蜂鸣器 + LCD 始终工作 ====
  for (uint8_t i = 0; i < 8; ++i) {
    int level = digitalRead(buttonPins[i]);
    if (level != lastLevel[i] && (now - lastDebounceMs[i]) > DEBOUNCE_MS) {
      lastDebounceMs[i] = now;

      if (level == ACTIVE_LEVEL && lastLevel[i] != ACTIVE_LEVEL) {
        // 1) 蜂鸣
        buzzerStart(notesHz[i], 120);

        // 2) LCD 音名 & 背光
        char line[17];
        snprintf(line, sizeof(line), "Note: %-3s     ", noteNames[i]);
        JHD1313M1::printLine(0, line);

        uint8_t r,g,b;
        hsvToRgb((float)hues[i], SAT, VAL, r, g, b);
        JHD1313M1::setRGB(r/2, g/2, b/2);

        // 3) 只有在 O（按键模式）时，才让灯带跟随按键变色
        if (digitalRead(modeSwitchPin) == HIGH) { // O 模式
          set_only_block_rgb(i, r, g, b);
          hues[i] = (hues[i] + HUE_STEP) % 360;
        }
      }

      lastLevel[i] = level;
    }
  }

  // ==== I（自动）模式：整条灯每秒切色 ====
  if (digitalRead(modeSwitchPin) == LOW) { // I 模式
    if (lastAutoMs == 0 || (now - lastAutoMs) >= AUTO_INTERVAL_MS) {
      lastAutoMs = now;
      uint8_t r,g,b;
      hsvToRgb((float)autoHue, SAT, VAL, r, g, b);
      set_all_rgb(r, g, b);

      // 背光也同步变一点（不影响你的LCD文字逻辑）
      JHD1313M1::setRGB(r/2, g/2, b/2);

      autoHue = (autoHue + 30) % 360;
    }
  }

  buzzerTask(); // 非阻塞结束蜂鸣
}
