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

// SU-03T 語音模組接腳
#define TTS_TX_PIN 18  // ESP32 TX -> SU-03T RX (I11)
#define TTS_RX_PIN 19  // ESP32 RX -> SU-03T TX (I12)

// 建立硬體序列埠給 SU-03T
HardwareSerial ttsSerial(2);  // 使用 UART2

// 燈光狀態（true=亮，false=暗）
bool redLedState = false;
bool greenLedState = false;
bool blueLedState = false;

// 上次按鈕狀態（用於偵測按下瞬間）
bool lastButton2State = LOW;  // 黑色按鈕
bool lastButton3State = LOW;
bool lastButton4State = LOW;
bool lastButton5State = LOW;

// 燈光秀狀態
enum ShowState {
  NORMAL,           // 正常模式
  WAITING,          // 等待3秒
  LIGHT_SHOW        // 燈光秀進行中
};

ShowState currentState = NORMAL;
unsigned long stateStartTime = 0;
bool allLightsWereOn = false;

// RGB燈條控制函數（共陽極設計，數值反轉）
void setRGB(int red, int green, int blue) {
  ledcWrite(PWM_CHANNEL_R, 255 - red);
  ledcWrite(PWM_CHANNEL_G, 255 - green);
  ledcWrite(PWM_CHANNEL_B, 255 - blue);
}

// 彩虹色彩計算（輸入0-255，輸出RGB）
void getRainbowColor(int position, int &r, int &g, int &b) {
  position = position % 256;
  if (position < 85) {
    r = 255 - position * 3;
    g = position * 3;
    b = 0;
  } else if (position < 170) {
    position -= 85;
    r = 0;
    g = 255 - position * 3;
    b = position * 3;
  } else {
    position -= 170;
    r = position * 3;
    g = 0;
    b = 255 - position * 3;
  }
}

// 燈光秀主函數
void runLightShow(unsigned long elapsedTime) {
  int r = 0, g = 0, b = 0;
  
  if (elapsedTime < 3000) {
    // 階段1：彩虹循環（0-3秒）
    int colorPos = (elapsedTime * 256 / 3000) % 256;
    getRainbowColor(colorPos, r, g, b);
    
  } else if (elapsedTime < 6000) {
    // 階段2：快速彩虹（3-6秒）
    int colorPos = ((elapsedTime - 3000) * 512 / 3000) % 256;
    getRainbowColor(colorPos, r, g, b);
    
  } else if (elapsedTime < 8000) {
    // 階段3：頻閃派對模式（6-8秒）
    if ((elapsedTime / 100) % 2 == 0) {
      int colorPos = (elapsedTime / 50) % 256;
      getRainbowColor(colorPos, r, g, b);
    } else {
      r = g = b = 0;
    }
    
  } else if (elapsedTime < 10000) {
    // 階段4：呼吸燈淡出（8-10秒）
    int fadeTime = elapsedTime - 8000;
    int brightness = 255 - (fadeTime * 255 / 2000);
    brightness = max(0, brightness);
    
    int colorPos = (elapsedTime / 10) % 256;
    getRainbowColor(colorPos, r, g, b);
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;
  }
  
  setRGB(r, g, b);
}

// SU-03T 語音合成函數（嘗試多種格式）
void speakText(const char* text) {
  // 格式1：標準格式加換行
  ttsSerial.print("[");
  ttsSerial.print(text);
  ttsSerial.println("]");
  
  Serial.print("🔊 播放語音（格式1）：");
  Serial.println(text);
  
  delay(100);
  
  // 格式2：加上前導碼
  ttsSerial.write(0xFD);
  ttsSerial.write((uint8_t)0x00);
  ttsSerial.write((uint8_t)strlen(text) + 2);
  ttsSerial.write((uint8_t)0x01);
  ttsSerial.write((uint8_t)0x01);
  ttsSerial.print(text);
  
  Serial.println("🔊 播放語音（格式2）：已發送帶前導碼指令");
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
  
  // 初始化 SU-03T 語音模組
  ttsSerial.begin(9600, SERIAL_8N1, TTS_RX_PIN, TTS_TX_PIN);
  delay(500);  // 等待模組初始化
  Serial.println("SU-03T 語音模組已初始化");
  
  Serial.println("按鈕配置：");
  Serial.println("  紅色按鈕 -> GPIO 12 (B7) -> 紅色燈光");
  Serial.println("  綠色按鈕 -> GPIO 33 (B12) -> 綠色燈光");
  Serial.println("  藍色按鈕 -> GPIO 32 (B13) -> 藍色燈光");
  Serial.println("  黑色按鈕 -> GPIO 14 (B8) -> 測試語音");
  Serial.println("  （黃色按鈕暫不作用）");
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
  unsigned long currentTime = millis();
  
  // 根據當前狀態執行不同邏輯
  if (currentState == NORMAL) {
    // 正常模式：處理按鈕輸入
    bool button2Current = (digitalRead(BUTTON_2) == HIGH);  // 黑色按鈕
    bool button3Current = (digitalRead(BUTTON_3) == HIGH);
    bool button4Current = (digitalRead(BUTTON_4) == HIGH);
    bool button5Current = (digitalRead(BUTTON_5) == HIGH);
    
    // 偵測黑色按鈕按下（測試語音）
    if (button2Current == HIGH && lastButton2State == LOW) {
      Serial.println("[黑色按鈕] 測試語音播放");
      speakText("測試成功，語音模組正常運作");
      delay(DEBOUNCE_DELAY);
    }
    lastButton2State = button2Current;
    
    // 偵測紅色按鈕按下瞬間（從LOW變HIGH）
    if (button3Current == HIGH && lastButton3State == LOW) {
      redLedState = !redLedState;
      Serial.print("[紅色按鈕] 紅燈 -> ");
      Serial.println(redLedState ? "開啟" : "關閉");
      delay(DEBOUNCE_DELAY);
    }
    lastButton3State = button3Current;
    
    // 偵測綠色按鈕按下瞬間
    if (button4Current == HIGH && lastButton4State == LOW) {
      greenLedState = !greenLedState;
      Serial.print("[綠色按鈕] 綠燈 -> ");
      Serial.println(greenLedState ? "開啟" : "關閉");
      delay(DEBOUNCE_DELAY);
    }
    lastButton4State = button4Current;
    
    // 偵測藍色按鈕按下瞬間
    if (button5Current == HIGH && lastButton5State == LOW) {
      blueLedState = !blueLedState;
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
    
    // 檢查是否三燈全亮
    bool allLightsOn = redLedState && greenLedState && blueLedState;
    if (allLightsOn && !allLightsWereOn) {
      // 三燈剛剛全亮，進入等待階段
      currentState = WAITING;
      stateStartTime = currentTime;
      allLightsWereOn = true;
      Serial.println("========================================");
      Serial.println("🎉 三燈全亮！倒數3秒後開始燈光秀...");
      Serial.println("========================================");
    }
    if (!allLightsOn) {
      allLightsWereOn = false;
    }
    
  } else if (currentState == WAITING) {
    // 等待階段：倒數3秒，期間閃爍提示
    unsigned long elapsed = currentTime - stateStartTime;
    
    // 閃爍效果（每0.5秒切換）
    if ((elapsed / 500) % 2 == 0) {
      setRGB(255, 255, 255);  // 全亮
    } else {
      setRGB(0, 0, 0);  // 全暗
    }
    
    // 每秒顯示倒數
    static int lastSecond = -1;
    int currentSecond = 3 - (elapsed / 1000);
    if (currentSecond != lastSecond && currentSecond >= 0) {
      Serial.print("倒數：");
      Serial.println(currentSecond);
      lastSecond = currentSecond;
    }
    
    // 3秒後進入燈光秀
    if (elapsed >= 3000) {
      currentState = LIGHT_SHOW;
      stateStartTime = currentTime;
      Serial.println("========================================");
      Serial.println("✨ 燈光秀開始！");
      Serial.println("========================================");
    }
    
  } else if (currentState == LIGHT_SHOW) {
    // 燈光秀階段
    unsigned long elapsed = currentTime - stateStartTime;
    
    if (elapsed < 10000) {
      // 執行燈光秀
      runLightShow(elapsed);
    } else {
      // 燈光秀結束，全暗並重置
      setRGB(0, 0, 0);
      redLedState = false;
      greenLedState = false;
      blueLedState = false;
      allLightsWereOn = false;
      currentState = NORMAL;
      Serial.println("========================================");
      Serial.println("🌙 燈光秀結束，所有燈已重置");
      Serial.println("========================================");
    }
  }
  
  delay(10);  // 短暫延遲，避免CPU空轉
}
