#include <Arduino.h>
#include <SPIFFS.h>
#include "BluetoothA2DPSource.h"

// 藍牙 A2DP Source
BluetoothA2DPSource a2dp_source;

// 音檔資訊
File audioFile;
bool audioFileReady = false;
bool isPlaying = false;

// 音檔列表（依類別分類）
String dadFiles[10];   // Dad 系列音檔
String momFiles[10];   // Mom 系列音檔
String sxFiles[10];    // SX 系列音檔
int dadCount = 0;
int momCount = 0;
int sxCount = 0;

// WAV 檔案標頭資訊（跳過前 44 bytes）
const int WAV_HEADER_SIZE = 44;

// 音頻緩衝區（用於提高讀取效能）
#define AUDIO_BUFFER_SIZE 512
uint8_t audioBuffer[AUDIO_BUFFER_SIZE];
int bufferIndex = 0;
int bufferSize = 0;

// 重採樣參數（8kHz -> 44.1kHz）
// 採樣率比例：44100 / 8000 = 5.5125
#define SRC_SAMPLE_RATE 8000
#define DST_SAMPLE_RATE 44100
float resamplePosition = 0.0;
int16_t lastSample = 0;

// 藍牙連接狀態
bool bluetoothConnected = false;

// 定義5個按鈕接腳（B側 - 輸入）
#define BUTTON_1 13  // B5 - 黃色按鈕（直接觸發抽籤）
#define BUTTON_2 14  // B8 - 黑色按鈕（保留未使用）
#define BUTTON_3 12  // B7 - 紅色按鈕（控制紅燈）
#define BUTTON_4 33  // B12 - 綠色按鈕（控制綠燈）
#define BUTTON_5 32  // B13 - 藍色按鈕（控制藍燈）

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
bool lastButton1State = LOW;  // 黃色按鈕
bool lastButton3State = LOW;  // 紅色按鈕
bool lastButton4State = LOW;  // 綠色按鈕
bool lastButton5State = LOW;  // 藍色按鈕

// 燈光秀狀態
enum ShowState {
  NORMAL,           // 正常模式
  WAITING,          // 等待3秒
  LIGHT_SHOW,       // 燈光秀進行中
  LOTTERY           // 抽籤播放
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

// 從緩衝區讀取一個樣本
int16_t readSample() {
  // 如果緩衝區空了，從檔案讀取新資料
  if (bufferIndex >= bufferSize) {
    if (audioFile.available()) {
      bufferSize = audioFile.read(audioBuffer, AUDIO_BUFFER_SIZE);
      bufferIndex = 0;
    } else {
      return 0;  // 檔案結束
    }
  }
  
  // 從緩衝區讀取 16-bit PCM 資料（小端序）
  if (bufferIndex + 1 < bufferSize) {
    uint8_t low = audioBuffer[bufferIndex++];
    uint8_t high = audioBuffer[bufferIndex++];
    return (int16_t)((high << 8) | low);
  }
  
  return 0;
}

// 藍牙音頻資料回調函數（使用重採樣）
int32_t get_sound_data(Frame *frame, int32_t frame_count) {
  if (!audioFileReady || !audioFile || !isPlaying) {
    // 沒有音檔或不在播放狀態，返回靜音
    for (int i = 0; i < frame_count; i++) {
      frame[i].channel1 = 0;
      frame[i].channel2 = 0;
    }
    return frame_count;
  }

  // 重採樣比例：源樣本速度相對於輸出的比例
  // 16000 / 44100 = 0.3628，表示每個輸出樣本前進 0.3628 個源樣本
  float ratio = (float)SRC_SAMPLE_RATE / (float)DST_SAMPLE_RATE;
  
  // 生成輸出樣本
  for (int i = 0; i < frame_count; i++) {
    // 檢查是否需要讀取新的源樣本
    if (resamplePosition >= 1.0) {
      // 讀取新樣本
      int16_t sample = readSample();
      
      // 檢查是否檔案結束
      if (sample == 0 && !audioFile.available() && bufferIndex >= bufferSize) {
        // 檔案結束，停止播放
        isPlaying = false;
        audioFile.close();
        Serial.println("✅ 播放完成");
        setRGB(0, 255, 0);  // 綠色表示藍牙連接但未播放
        
        // 填充剩餘 frame 為靜音
        for (int j = i; j < frame_count; j++) {
          frame[j].channel1 = 0;
          frame[j].channel2 = 0;
        }
        return frame_count;
      }
      
      lastSample = sample;
      resamplePosition -= 1.0;
    }
    
    // 使用當前樣本（重複使用以達到降速效果）
    // 單聲道音檔，兩個聲道播放相同內容
    frame[i].channel1 = lastSample;
    frame[i].channel2 = lastSample;
    
    // 更新重採樣位置
    resamplePosition += ratio;
  }
  
  return frame_count;
}

// 藍牙連接狀態回調
void connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
  if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
    bluetoothConnected = true;
    Serial.println("✅ 藍牙已連接到 Bose 喇叭");
    setRGB(0, 255, 0);  // 綠色表示藍牙連接成功
  } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    bluetoothConnected = false;
    Serial.println("❌ 藍牙已斷開");
    setRGB(255, 0, 0);  // 紅色表示藍牙斷開
  }
}

// 掃描 SPIFFS 並分類音檔
void scanAudioFiles() {
  Serial.println("\n【掃描音檔】");
  
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  dadCount = 0;
  momCount = 0;
  sxCount = 0;
  
  while (file) {
    String fileName = String(file.name());
    
    // 只處理 .wav 檔案
    if (fileName.endsWith(".wav")) {
      Serial.print("  發現音檔: ");
      Serial.println(fileName);
      
      // 根據檔名前綴分類（檔名可能有或沒有 / 前綴）
      if ((fileName.startsWith("/Dad_") || fileName.startsWith("Dad_")) && dadCount < 10) {
        dadFiles[dadCount++] = fileName;
        Serial.println("    → 歸類為 Dad 系列");
      } else if ((fileName.startsWith("/Mom_") || fileName.startsWith("Mom_")) && momCount < 10) {
        momFiles[momCount++] = fileName;
        Serial.println("    → 歸類為 Mom 系列");
      } else if ((fileName.startsWith("/SX_") || fileName.startsWith("SX_")) && sxCount < 10) {
        sxFiles[sxCount++] = fileName;
        Serial.println("    → 歸類為 SX 系列");
      }
    }
    
    file = root.openNextFile();
  }
  
  // 顯示統計
  Serial.println("\n📊 音檔統計：");
  Serial.print("  Dad 系列: ");
  Serial.print(dadCount);
  Serial.println(" 個");
  Serial.print("  Mom 系列: ");
  Serial.print(momCount);
  Serial.println(" 個");
  Serial.print("  SX 系列: ");
  Serial.print(sxCount);
  Serial.println(" 個");
  
  // 檢查是否有音檔
  if (dadCount > 0 || momCount > 0 || sxCount > 0) {
    audioFileReady = true;
    Serial.println("✅ 音檔掃描完成\n");
  } else {
    Serial.println("❌ 沒有找到任何音檔\n");
  }
}

// 播放指定音檔
void playAudioFile(String fileName) {
  if (isPlaying) {
    Serial.println("⚠️  正在播放中，請稍後再試");
    return;
  }
  
  Serial.print("🎵 開始播放: ");
  Serial.println(fileName);
  
  // 確保檔案路徑有 / 前綴
  if (!fileName.startsWith("/")) {
    fileName = "/" + fileName;
  }
  
  // 開啟音檔
  audioFile = SPIFFS.open(fileName, "r");
  if (audioFile) {
    // 跳過 WAV 標頭（44 bytes）
    audioFile.seek(WAV_HEADER_SIZE);
    
    // 初始化緩衝區和重採樣參數
    bufferIndex = 0;
    bufferSize = 0;
    resamplePosition = 1.0;
    lastSample = 0;
    
    isPlaying = true;
    setRGB(0, 0, 255);  // 藍色表示正在播放
    
    Serial.println("✅ 音檔已開啟，開始串流（16kHz -> 44.1kHz）...");
  } else {
    Serial.print("❌ 無法開啟音檔: ");
    Serial.println(fileName);
  }
}

// 抽籤選擇音檔（不立即播放）
String selectAudioFile() {
  if (!audioFileReady) {
    Serial.println("⚠️  沒有可用的音檔");
    return "";
  }
  
  Serial.println("\n🎲 開始抽籤...");
  
  // 隨機選擇類別（0=Dad, 1=Mom, 2=SX）
  int category = random(0, 3);
  String selectedFile = "";
  
  if (category == 0 && dadCount > 0) {
    // Dad 系列
    int index = random(0, dadCount);
    selectedFile = dadFiles[index];
    Serial.println("🎯 抽中 Dad 系列");
  } else if (category == 1 && momCount > 0) {
    // Mom 系列
    int index = random(0, momCount);
    selectedFile = momFiles[index];
    Serial.println("🎯 抽中 Mom 系列");
  } else if (category == 2 && sxCount > 0) {
    // SX 系列
    int index = random(0, sxCount);
    selectedFile = sxFiles[index];
    Serial.println("🎯 抽中 SX 系列");
  } else {
    // 如果選中的類別沒有音檔，隨便選一個有音檔的類別
    Serial.println("⚠️  該類別無音檔，重新選擇...");
    
    if (dadCount > 0) {
      int index = random(0, dadCount);
      selectedFile = dadFiles[index];
      Serial.println("🎯 抽中 Dad 系列（備選）");
    } else if (momCount > 0) {
      int index = random(0, momCount);
      selectedFile = momFiles[index];
      Serial.println("🎯 抽中 Mom 系列（備選）");
    } else if (sxCount > 0) {
      int index = random(0, sxCount);
      selectedFile = sxFiles[index];
      Serial.println("🎯 抽中 SX 系列（備選）");
    }
  }
  
  return selectedFile;
}

void setup() {
  // 初始化序列埠
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("========================================");
  Serial.println("ESP32 家庭任務提醒機");
  Serial.println("========================================");
  
  // 設定5個按鈕為輸入模式
  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);
  pinMode(BUTTON_4, INPUT);
  pinMode(BUTTON_5, INPUT);
  
  // 設定 LED PWM
  ledcSetup(PWM_CHANNEL_R, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_G, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  
  ledcAttachPin(RGB_R_PIN, PWM_CHANNEL_R);
  ledcAttachPin(RGB_G_PIN, PWM_CHANNEL_G);
  ledcAttachPin(RGB_B_PIN, PWM_CHANNEL_B);
  
  setRGB(0, 0, 0);  // 初始全暗
  
  // 初始化隨機數種子
  randomSeed(analogRead(0));
  
  // ========== 階段 1：初始化 SPIFFS ==========
  Serial.println("\n【階段 1】初始化 SPIFFS...");
  
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ SPIFFS 初始化失敗！");
    setRGB(255, 0, 0);  // 紅色表示錯誤
    while (1) { delay(1000); }
  }
  
  Serial.println("✅ SPIFFS 初始化成功");
  
  // 掃描並分類音檔
  scanAudioFiles();
  
  if (!audioFileReady) {
    Serial.println("❌ 沒有找到任何音檔");
    setRGB(255, 0, 0);  // 紅色表示錯誤
    while (1) { delay(1000); }
  }
  
  // ========== 階段 2：初始化藍牙 ==========
  Serial.println("\n【階段 2】初始化藍牙 A2DP...");
  Serial.println("   使用軟體重採樣：16kHz -> 44.1kHz");
  
  // 設定連接狀態回調
  a2dp_source.set_on_connection_state_changed(connection_state_changed);
  
  // 開始藍牙，嘗試連接到 Bose 喇叭
  Serial.println("🔍 正在搜尋 'Bose Mini II SoundLink'...");
  Serial.println("   請確保喇叭已開啟並進入配對模式！");
  
  a2dp_source.start("Bose Mini II SoundLink", get_sound_data);
  
  Serial.println("✅ 藍牙 A2DP 已啟動");
  Serial.println("   等待連接中...");
  
  // 等待連接（最多 10 秒）
  int waitCount = 0;
  while (!bluetoothConnected && waitCount < 100) {
    delay(100);
    waitCount++;
    if (waitCount % 10 == 0) {
      Serial.print(".");
    }
  }
  
  if (bluetoothConnected) {
    Serial.println("\n✅ 藍牙連接成功！");
    setRGB(0, 255, 0);  // 綠色表示藍牙連接成功
  } else {
    Serial.println("\n⚠️  藍牙尚未連接（可能需要手動配對）");
    Serial.println("   請在喇叭的藍牙設定中選擇 'ESP32'");
    setRGB(255, 255, 0);  // 黃色表示等待連接
  }
  
  // ========== 階段 3：系統就緒 ==========
  Serial.println("\n【階段 3】系統就緒");
  Serial.println("========================================");
  Serial.println("按鈕配置：");
  Serial.println("  紅色按鈕 (GPIO 12) -> 切換紅燈");
  Serial.println("  綠色按鈕 (GPIO 33) -> 切換綠燈");
  Serial.println("  藍色按鈕 (GPIO 32) -> 切換藍燈");
  Serial.println("  黃色按鈕 (GPIO 13) -> 直接抽籤播放");
  Serial.println("");
  Serial.println("✨ 特殊模式：三燈全亮 -> 3秒閃爍 -> 10秒燈光秀 -> 抽籤播放");
  Serial.println("========================================\n");
}

void loop() {
  unsigned long currentTime = millis();
  
  // 根據當前狀態執行不同邏輯
  if (currentState == NORMAL) {
    // ========== 正常模式：處理按鈕輸入 ==========
    
    // 讀取按鈕狀態
    bool button1Current = (digitalRead(BUTTON_1) == HIGH);  // 黃色按鈕（直接抽籤）
    bool button3Current = (digitalRead(BUTTON_3) == HIGH);  // 紅色按鈕
    bool button4Current = (digitalRead(BUTTON_4) == HIGH);  // 綠色按鈕
    bool button5Current = (digitalRead(BUTTON_5) == HIGH);  // 藍色按鈕
    
    // 偵測黃色按鈕按下（直接觸發抽籤）
    if (button1Current == HIGH && lastButton1State == LOW) {
      if (bluetoothConnected && audioFileReady) {
        Serial.println("[黃色按鈕] 直接觸發抽籤");
        String selectedFile = selectAudioFile();
        if (selectedFile != "") {
          playAudioFile(selectedFile);
        }
      } else {
        Serial.println("⚠️  藍牙未連接或無音檔，無法播放");
      }
      delay(DEBOUNCE_DELAY);
    }
    lastButton1State = button1Current;
    
    // 偵測紅色按鈕按下瞬間
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
    
    // 根據燈光狀態設定RGB（只在不播放時）
    if (!isPlaying) {
      int red = redLedState ? 255 : 0;
      int green = greenLedState ? 255 : 0;
      int blue = blueLedState ? 255 : 0;
      setRGB(red, green, blue);
    }
    
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
    // ========== 等待階段：倒數3秒，閃爍提示 ==========
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
    // ========== 燈光秀階段：10秒彩虹特效 ==========
    unsigned long elapsed = currentTime - stateStartTime;
    
    if (elapsed < 10000) {
      // 執行燈光秀
      runLightShow(elapsed);
    } else {
      // 燈光秀結束，進入抽籤階段
      currentState = LOTTERY;
      Serial.println("========================================");
      Serial.println("🎲 燈光秀結束，開始抽籤...");
      Serial.println("========================================");
    }
    
  } else if (currentState == LOTTERY) {
    // ========== 抽籤階段：選擇並播放音檔 ==========
    
    if (bluetoothConnected && audioFileReady) {
      String selectedFile = selectAudioFile();
      if (selectedFile != "") {
        playAudioFile(selectedFile);
      }
    } else {
      Serial.println("⚠️  藍牙未連接或無音檔，跳過播放");
    }
    
    // 重置所有狀態，回到正常模式
    setRGB(0, 0, 0);
    redLedState = false;
    greenLedState = false;
    blueLedState = false;
    allLightsWereOn = false;
    currentState = NORMAL;
    
    Serial.println("========================================");
    Serial.println("🌙 所有燈已重置，回到正常模式");
    Serial.println("========================================");
  }
  
  delay(10);  // 短暫延遲，避免CPU空轉
}
