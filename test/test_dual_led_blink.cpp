#include <Arduino.h>

// 定義 LED 接腳
#define LED_PIN_1 5
#define LED_PIN_2 18

void setup() {
  // 初始化序列埠（用於除錯）
  Serial.begin(115200);
  
  // 設定兩個 LED 腳位為輸出模式
  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  
  Serial.println("ESP32 雙 LED 交替控制程式啟動");
}

void loop() {
  // LED 1 亮，LED 2 滅
  digitalWrite(LED_PIN_1, HIGH);
  digitalWrite(LED_PIN_2, LOW);
  Serial.println("LED 1 亮 | LED 2 滅");
  delay(1000);  // 等待 1 秒
  
  // LED 1 滅，LED 2 亮
  digitalWrite(LED_PIN_1, LOW);
  digitalWrite(LED_PIN_2, HIGH);
  Serial.println("LED 1 滅 | LED 2 亮");
  delay(1000);  // 等待 1 秒
}