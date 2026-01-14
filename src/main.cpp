#include <Arduino.h>

// 定義5個按鈕接腳（B側 - 輸入）
#define BUTTON_1 13  // B5 - 黃色按鈕
#define BUTTON_2 14  // B8 - 黑色按鈕
#define BUTTON_3 12  // B7 - 紅色按鈕
#define BUTTON_4 33  // B12 - 綠色按鈕
#define BUTTON_5 32  // B13 - 藍色按鈕

// 定義RGB燈條接腳（I側 - 輸出）
#define RGB_R_PIN 16  // I8 - 紅色
#define RGB_G_PIN 17  // I9 - 綠色
#define RGB_B_PIN 5   // I10 - 藍色

// PWM通道定義
#define PWM_CHANNEL_R 0
#define PWM_CHANNEL_G 1
#define PWM_CHANNEL_B 2

// PWM設定
#define PWM_FREQ 5000      // PWM頻率 5kHz
#define PWM_RESOLUTION 8   // 8位元解析度 (0-255)

// 按鈕防彈跳時間（毫秒）
#define DEBOUNCE_DELAY 50

// 燈光狀態（true=亮，false=暗）
bool redLedState = false;
bool greenLedState = false;
bool blueLedState = false;

// 上次按鈕狀態（用於偵測按下瞬間）
bool lastButton3State = LOW;
bool lastButton4State = LOW;
bool lastButton5State = LOW;

// RGB燈條控制函數（共陽極設計，數值反轉）
void setRGB(int red, int green, int blue) {
  ledcWrite(PWM_CHANNEL_R, 255 - red);
  ledcWrite(PWM_CHANNEL_G, 255 - green);
  ledcWrite(PWM_CHANNEL_B, 255 - blue);
}

void setup() {
  // 初始化序列埠（用於除錯）
  Serial.begin(115200);
  Serial.println("========================================");
  Serial.println("ESP32 五按鈕控制RGB燈條程式");
  Serial.println("========================================");
  
  // 設定5個按鈕為輸入模式（不使用內建上拉，按鈕模組有自己的電路）
  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);
  pinMode(BUTTON_4, INPUT);
  pinMode(BUTTON_5, INPUT);
  
  // 設定PWM通道
  ledcSetup(PWM_CHANNEL_R, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_G, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  
  // 將GPIO綁定到PWM通道
  ledcAttachPin(RGB_R_PIN, PWM_CHANNEL_R);
  ledcAttachPin(RGB_G_PIN, PWM_CHANNEL_G);
  ledcAttachPin(RGB_B_PIN, PWM_CHANNEL_B);
  
  // 初始關閉RGB燈條
  setRGB(0, 0, 0);
  
  Serial.println("按鈕配置：");
  Serial.println("  紅色按鈕 -> GPIO 12 (B7) -> 紅色燈光");
  Serial.println("  綠色按鈕 -> GPIO 33 (B12) -> 綠色燈光");
  Serial.println("  藍色按鈕 -> GPIO 32 (B13) -> 藍色燈光");
  Serial.println("  （黃色、黑色按鈕暫不作用）");
  Serial.println("");
  Serial.println("RGB輸出：");
  Serial.println("  R -> GPIO 16 (I8)");
  Serial.println("  G -> GPIO 17 (I9)");
  Serial.println("  B -> GPIO 5 (I10)");
  Serial.println("========================================");
  Serial.println("請按任意按鈕，RGB燈條將會點亮");
  Serial.println("");
}

void loop() {
  // 讀取按鈕當前狀態
  bool button3Current = (digitalRead(BUTTON_3) == HIGH);
  bool button4Current = (digitalRead(BUTTON_4) == HIGH);
  bool button5Current = (digitalRead(BUTTON_5) == HIGH);
  
  // 偵測紅色按鈕按下瞬間（從LOW變HIGH）
  if (button3Current == HIGH && lastButton3State == LOW) {
    redLedState = !redLedState;  // 切換狀態
    Serial.print("[紅色按鈕] 紅燈 -> ");
    Serial.println(redLedState ? "開啟" : "關閉");
    delay(DEBOUNCE_DELAY);
  }
  lastButton3State = button3Current;
  
  // 偵測綠色按鈕按下瞬間
  if (button4Current == HIGH && lastButton4State == LOW) {
    greenLedState = !greenLedState;  // 切換狀態
    Serial.print("[綠色按鈕] 綠燈 -> ");
    Serial.println(greenLedState ? "開啟" : "關閉");
    delay(DEBOUNCE_DELAY);
  }
  lastButton4State = button4Current;
  
  // 偵測藍色按鈕按下瞬間
  if (button5Current == HIGH && lastButton5State == LOW) {
    blueLedState = !blueLedState;  // 切換狀態
    Serial.print("[藍色按鈕] 藍燈 -> ");
    Serial.println(blueLedState ? "開啟" : "關閉");
    delay(DEBOUNCE_DELAY);
  }
  lastButton5State = button5Current;
  
  // 根據燈光狀態設定RGB
  int red = redLedState ? 255 : 0;
  int green = greenLedState ? 255 : 0;
  int blue = blueLedState ? 255 : 0;
  setRGB(red, green, blue);
  
  delay(10);  // 短暫延遲，避免CPU空轉
}
