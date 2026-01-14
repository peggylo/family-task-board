#include <Arduino.h>

// 定義5個按鈕接腳（B側 - 輸入）
#define BUTTON_1 13  // B5 - 黃色按鈕
#define BUTTON_2 14  // B8 - 黑色按鈕
#define BUTTON_3 12  // B7 - 紅色按鈕
#define BUTTON_4 33  // B12 - 綠色按鈕
#define BUTTON_5 32  // B13 - 藍色按鈕

// 定義LED接腳（I側 - 輸出）
#define LED_PIN 4    // I7 - 測試用LED

// 按鈕防彈跳時間（毫秒）
#define DEBOUNCE_DELAY 50

void setup() {
  // 初始化序列埠（用於除錯）
  Serial.begin(115200);
  Serial.println("========================================");
  Serial.println("ESP32 五按鈕控制單LED測試程式");
  Serial.println("========================================");
  
  // 設定5個按鈕為輸入模式（不使用內建上拉，按鈕模組有自己的電路）
  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);
  pinMode(BUTTON_4, INPUT);
  pinMode(BUTTON_5, INPUT);
  
  // 設定LED為輸出模式
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // 初始LED關閉
  
  Serial.println("按鈕配置：");
  Serial.println("  黃色按鈕 -> GPIO 13 (B5)");
  Serial.println("  黑色按鈕 -> GPIO 14 (B8)");
  Serial.println("  紅色按鈕 -> GPIO 12 (B7)");
  Serial.println("  綠色按鈕 -> GPIO 33 (B12)");
  Serial.println("  藍色按鈕 -> GPIO 32 (B13)");
  Serial.println("");
  Serial.println("LED輸出：GPIO 4 (I7)");
  Serial.println("========================================");
  Serial.println("請按任意按鈕，LED將會點亮");
  Serial.println("");
}

void loop() {
  // 讀取5個按鈕的狀態（HIGH = 按下，LOW = 未按）
  bool button1Pressed = (digitalRead(BUTTON_1) == HIGH);
  bool button2Pressed = (digitalRead(BUTTON_2) == HIGH);
  bool button3Pressed = (digitalRead(BUTTON_3) == HIGH);
  bool button4Pressed = (digitalRead(BUTTON_4) == HIGH);
  bool button5Pressed = (digitalRead(BUTTON_5) == HIGH);
  
  // 檢查是否有任何一個按鈕被按下
  bool anyButtonPressed = button1Pressed || button2Pressed || 
                          button3Pressed || button4Pressed || 
                          button5Pressed;
  
  // 根據按鈕狀態控制LED
  if (anyButtonPressed) {
    digitalWrite(LED_PIN, HIGH);  // 點亮LED
    
    // 顯示哪個按鈕被按下
    Serial.print("按鈕按下：");
    if (button1Pressed) Serial.print("[黃色] ");
    if (button2Pressed) Serial.print("[黑色] ");
    if (button3Pressed) Serial.print("[紅色] ");
    if (button4Pressed) Serial.print("[綠色] ");
    if (button5Pressed) Serial.print("[藍色] ");
    Serial.println("-> LED亮");
    
    delay(DEBOUNCE_DELAY);  // 防彈跳延遲
  } else {
    digitalWrite(LED_PIN, LOW);   // 關閉LED
  }
  
  delay(10);  // 短暫延遲，避免CPU空轉
}
