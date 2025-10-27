
#include <SPI.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"    // 放在 config/ 或同目录，切记不要提交到仓库

// ========== MQTT 服务器与主题 ==========
static const char* MQTT_SERVER = "mqtt.cetools.org"; // 按需改
static const uint16_t MQTT_PORT = 1884;              // 1884(常用 TLS/非TLS见学校配置)
static const char* MQTT_CLIENT_ID = "mkr1010-8btn";  // 可改，需唯一
// 按你的实际主题改，比如：student/CASA0014/luminaire/<yourID>/rgb
static const char* MQTT_TOPIC = "student/CASA0014/luminaire/YOUR_ID/rgb";

// ====== LED 参数：发送 payload 给下游控制 72 颗灯 ======
static const uint16_t NUM_LEDS = 72;
static const uint16_t PAYLOAD_SIZE = NUM_LEDS * 3;
uint8_t RGBpayload[PAYLOAD_SIZE];

// ====== 8 个按钮配置（默认 NO 接法 + 内部上拉，按下=LOW）======
static const uint8_t BUTTON_PINS[8] = {2, 3, 4, 5, 6, 7, 8, 10};
static const int ACTIVE_LEVEL = LOW;          // NO + INPUT_PULLUP 时按下为 LOW
static const unsigned long DEBOUNCE_MS = 25;  // 机械消抖

int lastStableLevel[8];
unsigned long lastChangeMs[8];

// ====== 8 个颜色（按键 -> 整条灯统一该色）======
struct RGB { uint8_t r,g,b; };
const RGB PALETTE[8] = {
  {255,   0,   0},  // 按钮0 -> 红
  {255,  90,   0},  // 按钮1 -> 橙
  {255, 200,   0},  // 按钮2 -> 琥珀
  {  0, 255,   0},  // 按钮3 -> 绿
  {  0, 180, 255},  // 按钮4 -> 青
  {  0,  60, 255},  // 按钮5 -> 蓝
  {150,   0, 255},  // 按钮6 -> 紫
  {255,   0, 120},  // 按钮7 -> 洋红
};

// ====== WiFi / MQTT 账号（从 secrets 取）======
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

#ifdef SECRET_MQTTUSER
  const char* MQTT_USER = SECRET_MQTTUSER;
#else
  const char* MQTT_USER = "";
#endif

#ifdef SECRET_MQTTPASS
  const char* MQTT_PASS = SECRET_MQTTPASS;
#else
  const char* MQTT_PASS = "";
#endif

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ====== 函数声明 ======
void connectWiFi();
void connectMQTT();
void ensureConnections();
bool publishRGBAll(const RGB& c);

//（若你有订阅需求，可实现回调，这里只发布）
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // 保留：你的工程里如果需要处理下行，可在此填逻辑
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000) { ; }

  // 初始化按钮
  for (uint8_t i = 0; i < 8; ++i) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    lastStableLevel[i] = (ACTIVE_LEVEL == LOW) ? HIGH : LOW; // 初始视为未按
    lastChangeMs[i] = 0;
  }

  // WiFi & MQTT
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  connectWiFi();
  connectMQTT();

  Serial.println(F("== Ready: press any of the 8 buttons to publish 216-byte RGB payload =="));
  Serial.print(F("Topic: ")); Serial.println(MQTT_TOPIC);
}

void loop() {
  ensureConnections();     // 保活

  // 扫描 8 个键：只在“未按 -> 按下”的边沿触发一次发布
  for (uint8_t i = 0; i < 8; ++i) {
    int level = digitalRead(BUTTON_PINS[i]);
    unsigned long now = millis();

    if (level != lastStableLevel[i] && (now - lastChangeMs[i]) > DEBOUNCE_MS) {
      // 电平稳定改变
      lastChangeMs[i] = now;

      // 触发：未按 -> 按下
      if (level == ACTIVE_LEVEL && lastStableLevel[i] != ACTIVE_LEVEL) {
        const RGB &c = PALETTE[i];
        bool ok = publishRGBAll(c);
        Serial.print(F("[BTN] ")); Serial.print(i);
        Serial.print(F(" -> RGB(")); Serial.print(c.r); Serial.print(",");
        Serial.print(c.g); Serial.print(","); Serial.print(c.b);
        Serial.print(F(") publish ")); Serial.println(ok ? F("OK") : F("FAIL"));
      }

      lastStableLevel[i] = level;
    }
  }

  mqttClient.loop();       // 处理底层
}

// =========== 实现 ===========

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print(F("Connecting WiFi: "));
  Serial.print(ssid);

  WiFi.disconnect();
  int status = WL_IDLE_STATUS;
  unsigned long t0 = millis();

  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    unsigned long startTry = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTry) < 6000) {
      delay(100);
    }
    if (WiFi.status() == WL_CONNECTED) break;

    Serial.print('.');
    if (millis() - t0 > 30000) { // 30s 超时后再循环
      Serial.println(F("\nWiFi retry..."));
      t0 = millis();
    }
  }

  Serial.print(F("\nWiFi connected, IP="));
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  if (mqttClient.connected()) return;

  Serial.print(F("Connecting MQTT ")); Serial.print(MQTT_SERVER);
  Serial.print(F(":")); Serial.print(MQTT_PORT);

  uint8_t attempt = 0;
  while (!mqttClient.connected()) {
    bool ok;
    if (MQTT_USER && MQTT_USER[0] != '\0') {
      ok = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS);
    } else {
      ok = mqttClient.connect(MQTT_CLIENT_ID);
    }

    if (ok) break;

    Serial.print('.');
    attempt++;
    if (attempt % 10 == 0) {
      Serial.print(F(" rc=")); Serial.print(mqttClient.state());
      Serial.println(F(" retrying in 2s"));
    }
    delay(2000);
  }

  Serial.println(F("\nMQTT connected."));
  // 如需订阅，放在这里： mqttClient.subscribe("your/sub/topic");
}

void ensureConnections() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqttClient.connected()) connectMQTT();
}

// 将整条 72 灯统一成颜色 c，并按 216 字节发布到 MQTT_TOPIC
bool publishRGBAll(const RGB& c) {
  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    RGBpayload[i*3 + 0] = c.r;
    RGBpayload[i*3 + 1] = c.g;
    RGBpayload[i*3 + 2] = c.b;
  }
  if (!mqttClient.connected()) return false;
  // 第三个参数：长度 216 字节
  return mqttClient.publish(MQTT_TOPIC, RGBpayload, PAYLOAD_SIZE);
}
