#include <Arduino.h>

// 定義 LED 接腳（GPIO 05）
#define LED_PIN 5

void setup() {
  // 初始化序列埠
  Serial.begin(115200);
  
  // 設定 LED 腳位為輸出模式
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("ESP32 單色 LED 測試程式啟動");
  Serial.println("LED 接在 GPIO 05");
}

void loop() {
  // LED 亮
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED 亮");
  delay(1000);  // 等待 1 秒
  
  // LED 滅
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED 滅");
  delay(1000);  // 等待 1 秒
}
