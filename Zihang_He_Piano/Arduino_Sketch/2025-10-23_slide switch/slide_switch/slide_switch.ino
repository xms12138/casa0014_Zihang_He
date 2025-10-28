#include <Adafruit_NeoPixel.h>

#define LED_PIN     6       // 灯带数据引脚 (例如 D6)
#define NUM_LEDS    72
#define POT_PIN     A0      // 滑动电位器输出 OTA

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

float hue = 0.0;
unsigned long lastMs = 0;
unsigned long interval = 1000;  // 初始 1s 变一次

void setup() {
  Serial.begin(9600);
  pinMode(POT_PIN, INPUT);
  strip.begin();
  strip.show(); // 初始化清空灯带
}

void loop() {
  // 读取电位器（返回 0~1023）
  int potValue = analogRead(POT_PIN);

  // 把值映射到时间间隔 (100ms ~ 2000ms)
  interval = map(potValue, 0, 1023, 2000, 100);

  unsigned long now = millis();
  if (now - lastMs >= interval) {
    lastMs = now;
    hue += 15.0;
    if (hue >= 360.0) hue -= 360.0;

    // HSV 转 RGB
    uint8_t r, g, b;
    hsvToRgb(hue, 1.0, 0.8, r, g, b);

    // 整条灯带变色
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();

    Serial.print("potValue=");
    Serial.print(potValue);
    Serial.print("  interval(ms)=");
    Serial.println(interval);
  }
}

// 辅助函数：HSV 转 RGB
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
  r = (uint8_t)((r1 + m) * 255);
  g = (uint8_t)((g1 + m) * 255);
  b = (uint8_t)((b1 + m) * 255);
}
