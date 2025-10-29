#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"
#include <utility/wifi_drv.h>
#include <math.h>

/* ==================== 先定义效果类型（避免顺序问题） ==================== */
typedef uint8_t EffectType;
static const EffectType E_NONE    = 0;
static const EffectType E_RIPPLE  = 1;
static const EffectType E_COMET   = 2;
static const EffectType E_SPARKLE = 3;
static const EffectType E_WIPE    = 4;

/* ==================== WiFi / MQTT ==================== */
const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server   = "mqtt.cetools.org";
const int   mqtt_port     = 1884;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

String lightId    = "12";
String mqtt_topic = "student/CASA0014/luminaire/" + lightId;
String clientId   = "";

// 在 connections.ino 中实现
void startWifi();
void reconnectMQTT();
void callback(char* topic, byte* payload, unsigned int length);

/* ==================== 72 灯 payload ==================== */
const int num_leds     = 72;
const int payload_size = num_leds * 3;
byte RGBpayload[payload_size];
static uint8_t sparkBuf[payload_size] = {0};   // Sparkle 逐帧衰减缓冲

/* ==================== 8 个按钮 ==================== */
const uint8_t buttonPins[8] = {1,2,3,4,5,6,7,8};  // 如不稳可改 {2..9}
#define USE_NO 1
#if USE_NO
const int ACTIVE_LEVEL = LOW;
#else
const int ACTIVE_LEVEL = HIGH;
#endif
int lastLevel[8];
unsigned long lastDebounceMs[8] = {0};
const unsigned long DEBOUNCE_MS = 25;

/* ==================== 每组9灯独立色相 ==================== */
int   hues[8]  = {0,0,0,0,0,0,0,0};
const int   HUE_STEP = 45;
const float SAT = 1.0f;
const float VAL = 0.9f;

/* ==================== 蜂鸣器 ==================== */
const uint8_t pianoBuzzerPin = 9; // 钢琴被动蜂鸣器
bool pianoBuzzActive = false;
unsigned long pianoBuzzOffAt = 0;

const uint8_t metroBuzzerPin = 0; // 节拍器被动蜂鸣器（注意 D0/D1 可能是串口）
bool metroBuzzActive = false;
unsigned long metroBuzzOffAt = 0;

const uint16_t notesHz[8]   = {262,294,330,349,392,440,494,523}; // C4..C5
const char*    noteNames[8] = {"C4","D4","E4","F4","G4","A4","B4","C5"};

/* ==================== Grove LCD ==================== */
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
namespace JHD1313M1 { void setRGB_fix(){} } // 占位避免重复定义

/* ==================== 工具函数 ==================== */
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

void send_all_off() {
  if (!mqttClient.connected()) return;
  memset(RGBpayload, 0, payload_size);
  memset(sparkBuf,   0, payload_size);
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

/* ==================== 模式与滑动电位器 ==================== */
const uint8_t modeSwitchPin = 10;  // 拨到 I=LOW, 拨到 O=HIGH
const int potPin = A0;             // 滑动电位器

int lastModeLevel = HIGH;
unsigned long lastModeDebounce = 0;
const unsigned long MODE_DEBOUNCE_MS = 25;

int   bpm = 60;
unsigned long beatInterval = 1000;
unsigned long nextBeatAt = 0;
int hue_auto = 0;
uint8_t beatInBar = 0; // 4/4

int readStableBpm() {
  const uint8_t N = 5;
  uint16_t s[N];
  for (uint8_t i=0;i<N;i++) { s[i] = analogRead(potPin); }
  for (uint8_t i=0;i<N;i++) for (uint8_t j=i+1;j<N;j++) if (s[j]<s[i]) { uint16_t t=s[i]; s[i]=s[j]; s[j]=t; }
  uint16_t raw = s[N/2];
  static float ema = -1;
  if (ema < 0) ema = raw;
  const float alpha = 0.20f;
  ema = alpha*raw + (1.0f-alpha)*ema;
  int bpmNew = map((int)ema, 0, 4095, 40, 208);
  static int bpmShown = 120;
  const int deadband = 2;
  if (abs(bpmNew - bpmShown) >= deadband) bpmShown = bpmNew;
  return bpmShown;
}

/* ==================== 动画效果引擎（O 模式） ==================== */
struct EffectState {
  EffectType type = E_NONE;
  uint8_t    button = 0;
  int        originPixel = 0;
  int        hue = 0;
  unsigned long startMs = 0;
  unsigned long lastFrameMs = 0;
  bool       active = false;
  float      speed = 0.0f;   // 像素/秒
  float      width = 0.0f;   // 波宽/尾长
};
EffectState fx;                              // 简化：一次只跑一个效果
const unsigned long FX_FRAME_MS = 33;        // ~30fps

inline void payloadClear() { memset(RGBpayload, 0, payload_size); }
inline void payloadAddRGB(int p, uint8_t r, uint8_t g, uint8_t b) {
  if (p < 0 || p >= num_leds) return;
  int idx = p*3;
  RGBpayload[idx+0] = (uint8_t)min(255, RGBpayload[idx+0] + r);
  RGBpayload[idx+1] = (uint8_t)min(255, RGBpayload[idx+1] + g);
  RGBpayload[idx+2] = (uint8_t)min(255, RGBpayload[idx+2] + b);
}
int groupCenterFromButton(uint8_t btn) { return btn*9 + 4; } // 每组9灯的中心像素
EffectType effectForButton(uint8_t btn) {
  switch (btn % 4) {
    case 0: return E_RIPPLE;
    case 1: return E_COMET;
    case 2: return E_SPARKLE;
    default:return E_WIPE;
  }
}
void startEffect(uint8_t btn) {
  fx.type   = effectForButton(btn);
  fx.button = btn;
  fx.originPixel = groupCenterFromButton(btn);
  fx.hue    = hues[btn];
  fx.startMs = millis();
  fx.lastFrameMs = 0;
  fx.active = true;
  switch (fx.type) {
    case E_RIPPLE:  fx.speed = 60.0f; fx.width = 5.0f;  break;
    case E_COMET:   fx.speed = 90.0f; fx.width = 10.0f; break;
    case E_SPARKLE: fx.speed = 0.0f;  fx.width = 0.0f;  break;
    case E_WIPE:    fx.speed = 70.0f; fx.width = 12.0f; break;
    default: break;
  }
}
inline void decaySparkBuf(float k) {
  for (int i=0;i<payload_size;i++) sparkBuf[i] = (uint8_t)(sparkBuf[i] * k);
}
void renderEffects() {
  if (!fx.active) return;
  if (!mqttClient.connected()) return;

  unsigned long now = millis();
  if (fx.lastFrameMs && (now - fx.lastFrameMs) < FX_FRAME_MS) return;
  fx.lastFrameMs = now;

  float t = (now - fx.startMs) / 1000.0f;   // 秒
  uint8_t baseR, baseG, baseB;
  hsvToRgb((float)fx.hue, SAT, VAL, baseR, baseG, baseB);

  auto addWithGain = [&](int p, float gain){
    if (gain <= 0) return;
    uint8_t r = (uint8_t)min(255.0f, baseR * gain);
    uint8_t g = (uint8_t)min(255.0f, baseG * gain);
    uint8_t b = (uint8_t)min(255.0f, baseB * gain);
    payloadAddRGB(p, r, g, b);
  };

  if (fx.type != E_SPARKLE) payloadClear();

  switch (fx.type) {
    case E_RIPPLE: {
      float radius = fx.speed * t;
      for (int p = 0; p < num_leds; ++p) {
        float d = fabsf(p - fx.originPixel);
        float band = 1.0f - fabsf(d - radius) / fx.width; // 离“圈心”越近越亮
        addWithGain(p, max(0.0f, band));
      }
      if (t > 2.0f) fx.active = false;
    } break;

    case E_COMET: {
      bool toRight = (fx.button % 2 == 1);
      float head = fx.originPixel + (toRight ? +fx.speed*t : -fx.speed*t);
      for (int tail = 0; tail < (int)fx.width; ++tail) {
        int p = (int)roundf(head - (toRight ? tail : -tail));
        float gain = expf(-tail / (fx.width*0.5f));
        addWithGain(p, gain);
      }
      if (head < -10 || head > num_leds+10) fx.active = false;
    } break;

    case E_SPARKLE: {
      decaySparkBuf(0.85f);
      for (int k=0;k<8;k++){
        int jitter = random(-8, 9);
        int p = fx.originPixel + jitter;
        if (p < 0 || p >= num_leds) continue;
        float gain = random(70, 100) / 100.0f; // 0.70~1.00
        int idx = p*3;
        sparkBuf[idx+0] = (uint8_t)min(255.0f, sparkBuf[idx+0] + baseR * gain);
        sparkBuf[idx+1] = (uint8_t)min(255.0f, sparkBuf[idx+1] + baseG * gain);
        sparkBuf[idx+2] = (uint8_t)min(255.0f, sparkBuf[idx+2] + baseB * gain);
      }
      memcpy(RGBpayload, sparkBuf, payload_size);
      if (t > 1.5f) fx.active = false;
    } break;

    case E_WIPE: {
      float front = fx.speed * t;
      for (int p = 0; p < num_leds; ++p) {
        float d = fabsf(p - fx.originPixel);
        float leadGain  = 1.0f - (d - front)/fx.width;                       // 前沿
        float trailGain = expf(-max(0.0f, front - d)/ (fx.width*0.7f));      // 身后衰减
        float gain = max(0.0f, min(1.0f, max(leadGain, 0.0f) + 0.3f*trailGain));
        addWithGain(p, gain);
      }
      if (front > (num_leds + fx.width)) fx.active = false;
    } break;

    default: fx.active = false; break;
  }

  mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
}

/* ==================== setup / loop ==================== */
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

  analogReadResolution(12);          // 0..4095
  analogReference(AR_DEFAULT);       // 3.3V

  randomSeed(analogRead(A1));        // Sparkle 随机

  JHD1313M1::init();
  JHD1313M1::printLine(0, "Ready");
  JHD1313M1::printLine(1, "Press a key");

  startWifi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(2000);
  mqttClient.setCallback(callback);

  send_all_off();
}

void loop() {
  if (!mqttClient.connected()) reconnectMQTT();
  if (WiFi.status() != WL_CONNECTED) startWifi();
  mqttClient.loop();

  unsigned long now = millis();

  // 模式检测
  static int lastModeLevelLocal = HIGH;
  static unsigned long lastModeDebounceLocal = 0;
  int modeLevel = digitalRead(modeSwitchPin);
  if (modeLevel != lastModeLevelLocal && (now - lastModeDebounceLocal) > MODE_DEBOUNCE_MS) {
    lastModeDebounceLocal = now;
    lastModeLevelLocal = modeLevel;
    if (modeLevel == LOW) { // 切到 I 模式
      nextBeatAt = 0; beatInBar = 0; fx.active = false;
    }
  }

  // 钢琴系统（总是工作：蜂鸣 + LCD；O 模式触发动画）
  for (uint8_t i = 0; i < 8; ++i) {
    int level = digitalRead(buttonPins[i]);
    if (level != lastLevel[i] && (now - lastDebounceMs[i]) > DEBOUNCE_MS) {
      lastDebounceMs[i] = now;

      if (level == ACTIVE_LEVEL && lastLevel[i] != ACTIVE_LEVEL) {
        // 蜂鸣
        pianoBeepStart(notesHz[i], 120);

        // LCD
        char line[17];
        snprintf(line, sizeof(line), "Note: %-3s", noteNames[i]);
        JHD1313M1::printLine(0, line);

        uint8_t r,g,b;
        hsvToRgb((float)hues[i], SAT, VAL, r, g, b);
        JHD1313M1::setRGB(r/2, g/2, b/2);

        // O 模式触发动画（取代“点亮9颗”）
        if (modeLevel == HIGH) {
          startEffect(i);
          hues[i] = (hues[i] + HUE_STEP) % 360;   // 每次按下推进该键色相
        }
      }
      lastLevel[i] = level;
    }
  }

  // I 模式：节拍器逻辑
  if (modeLevel == LOW) {
    bpm = readStableBpm();
    beatInterval = 60000UL / bpm;

    if (now >= nextBeatAt) {
      nextBeatAt = now + beatInterval;

      if (beatInBar == 0) metroBeepStart(1600, 55);
      else                metroBeepStart(1200, 40);

      hue_auto = (hue_auto + 30) % 360;
      uint8_t r,g,b;
      hsvToRgb((float)hue_auto, 1.0, 0.9, r, g, b);
      set_all_rgb(r, g, b);

      beatInBar = (beatInBar + 1) % 4;

      char bpmLine[17];
      snprintf(bpmLine, sizeof(bpmLine), "BPM:%3d", bpm);
      JHD1313M1::printLine(1, bpmLine);
    }
  } else {
    // O 模式：持续驱动动画帧
    renderEffects();
  }

  // 关蜂鸣器
  buzzerTask();
}
