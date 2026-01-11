#include <Arduino.h>

// 定義 LED 接腳
#define LED_PIN 5

void setup() {
  // 初始化序列埠（用於除錯）
  Serial.begin(115200);
  
  // 設定 LED 腳位為輸出模式
  pinMode(LED_PIN, OUTPUT);
  
  Serial.println("ESP32 LED 控制程式啟動");
}

void loop() {
  // LED 亮起
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED 亮");
  delay(1000);  // 等待 1 秒
  
  // LED 熄滅
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED 滅");
  delay(1000);  // 等待 1 秒
}
