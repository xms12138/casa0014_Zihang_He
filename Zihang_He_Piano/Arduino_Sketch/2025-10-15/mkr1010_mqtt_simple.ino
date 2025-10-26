// // #include <SPI.h>
// // #include <WiFiNINA.h>
// // #include <PubSubClient.h>
// // #include "arduino_secrets.h"
// // #include <utility/wifi_drv.h>   // library to drive to RGB LED on the MKR1010

// // /*
// // **** please enter your sensitive data in the Secret tab/arduino_secrets.h
// // **** using format below
// // #define SECRET_SSID "ssid name"
// // #define SECRET_PASS "ssid password"
// // #define SECRET_MQTTUSER "user name - eg student"
// // #define SECRET_MQTTPASS "password";
// // */
// // const char* ssid          = SECRET_SSID;
// // const char* password      = SECRET_PASS;
// // const char* mqtt_username = SECRET_MQTTUSER;
// // const char* mqtt_password = SECRET_MQTTPASS;
// // const char* mqtt_server   = "mqtt.cetools.org";
// // const int   mqtt_port     = 1884;

// // // ===== 重要：如果 1884 需要 TLS，把下一行改为 WiFiSSLClient 并引入 <WiFiSSLClient.h> =====
// // // #include <WiFiSSLClient.h>
// // // WiFiSSLClient wifiClient;     // ← 若使用 TLS，请用这一行
// // WiFiClient wifiClient;          // ← 非 TLS（仅在 broker 允许时可用）

// // PubSubClient mqttClient(wifiClient);

// // // Make sure to update your lightid value below with the one you have been allocated
// // String lightId = "12"; // the topic id number or user number being used.

// // // Here we define the MQTT topic we will be publishing data to
// // String mqtt_topic = "student/CASA0014/luminaire/" + lightId;
// // String clientId = ""; // will set once i have mac address so that it is unique

// // // NeoPixel Configuration - we need to know this to know how to send messages to vespera
// // const int num_leds     = 72;
// // const int payload_size = num_leds * 3; // x3 for RGB

// // // Create the byte array to send in MQTT payload this stores all the colours
// // // in memory so that they can be accessed in for example the rainbow function
// // byte RGBpayload[payload_size];

// // // ===== 新增：HSV→RGB 转换（h:0..360, s/v:0..1）=====
// // void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
// //   float c = v * s;
// //   float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
// //   float m = v - c;

// //   float r1, g1, b1;
// //   if      (h < 60)  { r1 = c; g1 = x; b1 = 0; }
// //   else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
// //   else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
// //   else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
// //   else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
// //   else              { r1 = c; g1 = 0; b1 = x; }

// //   r = (uint8_t)((r1 + m) * 255.0f);
// //   g = (uint8_t)((g1 + m) * 255.0f);
// //   b = (uint8_t)((b1 + m) * 255.0f);
// // }

// // // ===== 新增：整条灯带设为同一颜色并发布 =====
// // void set_all_rgb(uint8_t r, uint8_t g, uint8_t b) {
// //   if (!mqttClient.connected()) {
// //     Serial.println("MQTT not connected, skip publish from set_all_rgb.");
// //     return;
// //   }
// //   for (int pixel = 0; pixel < num_leds; pixel++) {
// //     RGBpayload[pixel * 3 + 0] = r;
// //     RGBpayload[pixel * 3 + 1] = g;
// //     RGBpayload[pixel * 3 + 2] = b;
// //   }
// //   mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
// // }

// // // ====== 变色时序控制参数（每秒换色）======
// // unsigned long lastTick = 0;
// // int   hue              = 0;         // 0..359
// // const int   HUE_STEP   = 15;        // 每次推进 15°（360/15=24 步一圈）
// // const unsigned long PERIOD_MS = 1000; // 每 1 秒换色一次
// // const float SAT = 1.0f;             // 饱和度
// // const float VAL = 0.9f;             // 亮度

// // void setup() {
// //   Serial.begin(115200);
// //   // while (!Serial);
// //   Serial.println("Vespera");

// //   // print your MAC address:
// //   byte mac[6];
// //   WiFi.macAddress(mac);
// //   Serial.print("MAC address: ");
// //   printMacAddress(mac);

// //   Serial.print("This device is Vespera ");
// //   Serial.println(lightId);

// //   // Connect to WiFi
// //   startWifi();

// //   // Connect to MQTT broker
// //   mqttClient.setServer(mqtt_server, mqtt_port);
// //   mqttClient.setBufferSize(2000);
// //   mqttClient.setCallback(callback);

// //   Serial.println("Set-up complete");
// // }

// // void loop() {
// //   // Reconnect if necessary
// //   if (!mqttClient.connected()) {
// //     reconnectMQTT();
// //   }
// //   if (WiFi.status() != WL_CONNECTED) {
// //     startWifi();
// //   }
// //   // keep mqtt alive
// //   mqttClient.loop();

// //   // ====== 每秒整条灯带换一种颜色（同色）======
// //   unsigned long now = millis();
// //   if (now - lastTick >= PERIOD_MS) {
// //     lastTick = now;

// //     uint8_t r, g, b;
// //     hsvToRgb((float)hue, SAT, VAL, r, g, b);
// //     set_all_rgb(r, g, b);

// //     hue = (hue + HUE_STEP) % 360; // 下一种颜色

// //     // 调试输出
// //     Serial.print("HSV("); Serial.print(hue); Serial.print(") -> RGB(");
// //     Serial.print(r); Serial.print(","); Serial.print(g); Serial.print(","); Serial.print(b); Serial.println(")");
// //   }
// // }

// // // ======= 下面几段保留以备后续需要（当前不在 loop 中使用）=======

// // // Function to update the R, G, B values of a single LED pixel
// // // RGB can a value between 0-254, pixel is 0-71 for a 72 neopixel strip
// // void send_RGB_to_pixel(int r, int g, int b, int pixel) {
// //   if (mqttClient.connected()) {
// //     RGBpayload[pixel * 3 + 0] = (byte)r; // Red
// //     RGBpayload[pixel * 3 + 1] = (byte)g; // Green
// //     RGBpayload[pixel * 3 + 2] = (byte)b; // Blue
// //     mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
// //     Serial.println("Published whole byte array after updating a single pixel.");
// //   } else {
// //     Serial.println("MQTT mqttClient not connected, cannot publish from *send_RGB_to_pixel*.");
// //   }
// // }

// // void send_all_off() {
// //   if (mqttClient.connected()) {
// //     for (int pixel = 0; pixel < num_leds; pixel++) {
// //       RGBpayload[pixel * 3 + 0] = (byte)0;
// //       RGBpayload[pixel * 3 + 1] = (byte)0;
// //       RGBpayload[pixel * 3 + 2] = (byte)0;
// //     }
// //     mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
// //     Serial.println("Published an all zero (off) byte array.");
// //   } else {
// //     Serial.println("MQTT mqttClient not connected, cannot publish from *send_all_off*.");
// //   }
// // }

// // void send_all_random() {
// //   if (mqttClient.connected()) {
// //     for (int pixel = 0; pixel < num_leds; pixel++) {
// //       RGBpayload[pixel * 3 + 0] = (byte)random(50, 256); // Red - 256 is exclusive, so it goes up to 255
// //       RGBpayload[pixel * 3 + 1] = (byte)random(50, 256); // Green
// //       RGBpayload[pixel * 3 + 2] = (byte)random(50, 256); // Blue
// //     }
// //     mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
// //     Serial.println("Published an all random byte array.");
// //   } else {
// //     Serial.println("MQTT mqttClient not connected, cannot publish from *send_all_random*.");
// //   }
// // }

// // void printMacAddress(byte mac[]) {
// //   for (int i = 5; i >= 0; i--) {
// //     if (mac[i] < 16) {
// //       Serial.print("0");
// //     }
// //     Serial.print(mac[i], HEX);
// //     if (i > 0) {
// //       Serial.print(":");
// //     }
// //   }
// //   Serial.println();
// // }

// // Duncan Wilson Oct 2025 - v1 - MQTT messager to vespera
// // works with MKR1010

// #include <SPI.h>
// #include <WiFiNINA.h>
// #include <PubSubClient.h>
// #include "arduino_secrets.h"
// #include <utility/wifi_drv.h>   // library to drive to RGB LED on the MKR1010

// /*
// **** please enter your sensitive data in the Secret tab/arduino_secrets.h
// **** using format below
// #define SECRET_SSID "ssid name"
// #define SECRET_PASS "ssid password"
// #define SECRET_MQTTUSER "user name - eg student"
// #define SECRET_MQTTPASS "password";
// */
// const char* ssid          = SECRET_SSID;
// const char* password      = SECRET_PASS;
// const char* mqtt_username = SECRET_MQTTUSER;
// const char* mqtt_password = SECRET_MQTTPASS;
// const char* mqtt_server   = "mqtt.cetools.org";
// const int   mqtt_port     = 1884;

// // ===== 如需 TLS, 可改为 WiFiSSLClient 并包含 <WiFiSSLClient.h> =====
// // #include <WiFiSSLClient.h>
// // WiFiSSLClient wifiClient;
// WiFiClient wifiClient;

// PubSubClient mqttClient(wifiClient);

// // 你的专属主题 ID
// String lightId = "12";
// String mqtt_topic = "student/CASA0014/luminaire/" + lightId;
// String clientId = ""; // 由另一个文件在 startWifi() 中设置也可

// // ---- NeoPixel（Vespera 端）配置 ----
// const int num_leds     = 72;
// const int payload_size = num_leds * 3; // 216 bytes
// byte RGBpayload[payload_size];

// // ---- 触摸传感器引脚（TTP223 IO -> D6）----
// const int touchPin = 6;
// int lastTouchState = LOW;
// unsigned long lastDebounceMs = 0;
// const unsigned long debounceInterval = 200; // 200ms 消抖

// // ---- 颜色控制（触摸一次换一次色）----
// int   hue = 0;           // 0..359
// const int   HUE_STEP = 30;   // 每次触摸推进 30°
// const float SAT = 1.0f;      // 饱和度 0..1
// const float VAL = 0.9f;      // 亮度   0..1

// // ===== HSV→RGB =====
// void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b) {
//   float c = v * s;
//   float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
//   float m = v - c;

//   float r1, g1, b1;
//   if      (h < 60)  { r1 = c; g1 = x; b1 = 0; }
//   else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
//   else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
//   else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
//   else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
//   else              { r1 = c; g1 = 0; b1 = x; }

//   r = (uint8_t)((r1 + m) * 255.0f);
//   g = (uint8_t)((g1 + m) * 255.0f);
//   b = (uint8_t)((b1 + m) * 255.0f);
// }

// // ===== 整条设为同色并发布 =====
// void set_all_rgb(uint8_t r, uint8_t g, uint8_t b) {
//   if (!mqttClient.connected()) {
//     Serial.println("MQTT not connected, skip publish from set_all_rgb.");
//     return;
//   }
//   for (int pixel = 0; pixel < num_leds; pixel++) {
//     RGBpayload[pixel * 3 + 0] = r;
//     RGBpayload[pixel * 3 + 1] = g;
//     RGBpayload[pixel * 3 + 2] = b;
//   }
//   mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
// }

// // ===== 常用辅助（保留）=====
// void send_RGB_to_pixel(int r, int g, int b, int pixel) {
//   if (mqttClient.connected()) {
//     RGBpayload[pixel * 3 + 0] = (byte)r;
//     RGBpayload[pixel * 3 + 1] = (byte)g;
//     RGBpayload[pixel * 3 + 2] = (byte)b;
//     mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
//   }
// }
// void send_all_off() {
//   if (mqttClient.connected()) {
//     for (int pixel = 0; pixel < num_leds; pixel++) {
//       RGBpayload[pixel * 3 + 0] = 0;
//       RGBpayload[pixel * 3 + 1] = 0;
//       RGBpayload[pixel * 3 + 2] = 0;
//     }
//     mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
//   }
// }
// void send_all_random() {
//   if (mqttClient.connected()) {
//     for (int pixel = 0; pixel < num_leds; pixel++) {
//       RGBpayload[pixel * 3 + 0] = (byte)random(50, 256);
//       RGBpayload[pixel * 3 + 1] = (byte)random(50, 256);
//       RGBpayload[pixel * 3 + 2] = (byte)random(50, 256);
//     }
//     mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
//   }
// }
// void printMacAddress(byte mac[]) {
//   for (int i = 5; i >= 0; i--) {
//     if (mac[i] < 16) Serial.print("0");
//     Serial.print(mac[i], HEX);
//     if (i > 0) Serial.print(":");
//   }
//   Serial.println();
// }

// void setup() {
//   Serial.begin(115200);
//   // while (!Serial);

//   // 打印 MAC
//   byte mac[6];
//   WiFi.macAddress(mac);
//   Serial.print("MAC address: ");
//   printMacAddress(mac);
//   Serial.print("This device is Vespera ");
//   Serial.println(lightId);

//   // 触摸引脚
//   pinMode(touchPin, INPUT);   // TTP223 模块自带输出，通常无需上拉
//   lastTouchState = digitalRead(touchPin);

//   // 先连 WiFi
//   startWifi();

//   // MQTT
//   mqttClient.setServer(mqtt_server, mqtt_port);
//   mqttClient.setBufferSize(2000);
//   mqttClient.setCallback(callback);

//   Serial.println("Set-up complete");
// }

// void loop() {
//   // 网络保活
//   if (!mqttClient.connected()) {
//     reconnectMQTT();
//   }
//   if (WiFi.status() != WL_CONNECTED) {
//     startWifi();
//   }
//   mqttClient.loop();

//   // ===== 触摸检测：上升沿触发换色 =====
//   int reading = digitalRead(touchPin);       // TTP223 触摸时通常输出 HIGH
//   unsigned long now = millis();

//   // 基本消抖
//   if (reading != lastTouchState && (now - lastDebounceMs) > debounceInterval) {
//     lastDebounceMs = now;

//     if (reading == HIGH) { // 仅在触摸按下瞬间触发
//       // 计算下一种颜色
//       uint8_t r, g, b;
//       hsvToRgb((float)hue, SAT, VAL, r, g, b);
//       set_all_rgb(r, g, b);

//       Serial.print("Touch! Set hue="); Serial.print(hue);
//       Serial.print(" -> RGB("); Serial.print(r); Serial.print(",");
//       Serial.print(g); Serial.print(","); Serial.print(b); Serial.println(")");

//       hue = (hue + HUE_STEP) % 360; // 下次触摸用下一种色
//     }

//     lastTouchState = reading;
//   }
// }
// Duncan Wilson Oct 2025 - v1 - MQTT messager to vespera
// works with MKR1010

#include <SPI.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"
#include <utility/wifi_drv.h>   // MKR1010 板载 RGB LED 驱动

/*
**** please enter your sensitive data in the Secret tab/arduino_secrets.h
**** using format below
#define SECRET_SSID "ssid name"
#define SECRET_PASS "ssid password"
#define SECRET_MQTTUSER "user name - eg student"
#define SECRET_MQTTPASS "password";
*/
const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;
const char* mqtt_server   = "mqtt.cetools.org";
const int   mqtt_port     = 1884;

// ===== 如需 TLS，可改为 WiFiSSLClient 并包含 <WiFiSSLClient.h> =====
// #include <WiFiSSLClient.h>
// WiFiSSLClient wifiClient;
WiFiClient wifiClient;

PubSubClient mqttClient(wifiClient);

// ---- 你的专属主题 ID ----
String lightId = "12";
String mqtt_topic = "student/CASA0014/luminaire/" + lightId;
String clientId = ""; // 可在 startWifi() 里用 MAC 生成

// ---- NeoPixel（Vespera 端）配置 ----
const int num_leds     = 72;
const int payload_size = num_leds * 3; // 216 bytes
byte RGBpayload[payload_size];

// =====================
// 机械开关（NC/NO/COM）设置
// =====================
const int buttonPin = 6;      // D6 接 NO/NC 触点
#define USE_NO 1              // 1=使用 NO 接法（推荐）；0=使用 NC 接法
#if USE_NO
// NO: COM→GND, NO→D6, INPUT_PULLUP 下按下=LOW
const int ACTIVE_LEVEL = LOW;
#else
// NC: COM→GND, NC→D6, INPUT_PULLUP 下按下（被按断开）=HIGH
const int ACTIVE_LEVEL = HIGH;
#endif

int lastLevel = !ACTIVE_LEVEL;             // 初始为“未按”状态
unsigned long lastDebounceMs = 0;
const unsigned long debounceInterval = 25; // 25ms 机械消抖（可按需调整）

// ---- 颜色控制（按一下换一次色）----
int   hue = 0;                 // 0..359
const int   HUE_STEP = 30;     // 每次推进 30°
const float SAT = 1.0f;        // 饱和度
const float VAL = 0.9f;        // 亮度

// ===== HSV→RGB =====
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

// ===== 整条设为同色并发布 =====
void set_all_rgb(uint8_t r, uint8_t g, uint8_t b) {
  if (!mqttClient.connected()) {
    Serial.println("MQTT not connected, skip publish from set_all_rgb.");
    return;
  }
  for (int pixel = 0; pixel < num_leds; pixel++) {
    RGBpayload[pixel * 3 + 0] = r;
    RGBpayload[pixel * 3 + 1] = g;
    RGBpayload[pixel * 3 + 2] = b;
  }
  mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
}

// ===== 常用辅助（保留）=====
void send_RGB_to_pixel(int r, int g, int b, int pixel) {
  if (mqttClient.connected()) {
    RGBpayload[pixel * 3 + 0] = (byte)r;
    RGBpayload[pixel * 3 + 1] = (byte)g;
    RGBpayload[pixel * 3 + 2] = (byte)b;
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
  }
}
void send_all_off() {
  if (mqttClient.connected()) {
    for (int pixel = 0; pixel < num_leds; pixel++) {
      RGBpayload[pixel * 3 + 0] = 0;
      RGBpayload[pixel * 3 + 1] = 0;
      RGBpayload[pixel * 3 + 2] = 0;
    }
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
  }
}
void send_all_random() {
  if (mqttClient.connected()) {
    for (int pixel = 0; pixel < num_leds; pixel++) {
      RGBpayload[pixel * 3 + 0] = (byte)random(50, 256);
      RGBpayload[pixel * 3 + 1] = (byte)random(50, 256);
      RGBpayload[pixel * 3 + 2] = (byte)random(50, 256);
    }
    mqttClient.publish(mqtt_topic.c_str(), RGBpayload, payload_size);
  }
}
void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i > 0) Serial.print(":");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  // while (!Serial);

  // 打印 MAC
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
  Serial.print("This device is Vespera ");
  Serial.println(lightId);

  // 机械开关输入（上拉）
  pinMode(buttonPin, INPUT_PULLUP);
  lastLevel = digitalRead(buttonPin);

  // 先连 WiFi
  startWifi();

  // MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setBufferSize(2000);
  mqttClient.setCallback(callback);

  Serial.println("Set-up complete");
}

void loop() {
  // 网络保活
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  if (WiFi.status() != WL_CONNECTED) {
    startWifi();
  }
  mqttClient.loop();

  // ===== 机械开关：只在“按下的边沿”换色 =====
  int level = digitalRead(buttonPin);
  unsigned long now = millis();

  if (level != lastLevel && (now - lastDebounceMs) > debounceInterval) {
    lastDebounceMs = now;

    // 检测到“未按 -> 按下”的瞬间（依据 NO/NC 的 ACTIVE_LEVEL 自适应）
    if (level == ACTIVE_LEVEL && lastLevel != ACTIVE_LEVEL) {
      uint8_t r, g, b;
      hsvToRgb((float)hue, SAT, VAL, r, g, b);
      set_all_rgb(r, g, b);

      Serial.print("Button! Set hue=");
      Serial.print(hue);
      Serial.print(" -> RGB(");
      Serial.print(r); Serial.print(",");
      Serial.print(g); Serial.print(",");
      Serial.print(b); Serial.println(")");

      hue = (hue + HUE_STEP) % 360; // 准备下次颜色
    }

    lastLevel = level;
  }
}
