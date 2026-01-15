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

// 按鈕定義（保留一個按鈕用於觸發播放）
#define BUTTON_PLAY 13  // B5 - 黃色按鈕（觸發播放）
bool lastButtonState = LOW;

// LED 定義（用於狀態指示）
#define RGB_R_PIN 16  // I8 - 紅色（SPIFFS 狀態）
#define RGB_G_PIN 17  // I9 - 綠色（藍牙狀態）
#define RGB_B_PIN 5   // I10 - 藍色（播放狀態）

// PWM 設定
#define PWM_CHANNEL_R 0
#define PWM_CHANNEL_G 1
#define PWM_CHANNEL_B 2
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// RGB 控制（共陽極）
void setRGB(int red, int green, int blue) {
  ledcWrite(PWM_CHANNEL_R, 255 - red);
  ledcWrite(PWM_CHANNEL_G, 255 - green);
  ledcWrite(PWM_CHANNEL_B, 255 - blue);
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

// 隨機抽籤並播放
void lottery() {
  if (!bluetoothConnected) {
    Serial.println("⚠️  藍牙尚未連接，無法播放");
    return;
  }
  
  if (!audioFileReady) {
    Serial.println("⚠️  沒有可用的音檔");
    return;
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
  
  // 播放選中的音檔
  if (selectedFile != "") {
    playAudioFile(selectedFile);
  } else {
    Serial.println("❌ 無法選擇音檔");
  }
}

void setup() {
  // 初始化序列埠
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("========================================");
  Serial.println("ESP32 SPIFFS + 藍牙音頻測試");
  Serial.println("========================================");
  
  // 設定 LED PWM
  ledcSetup(PWM_CHANNEL_R, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_G, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_B, PWM_FREQ, PWM_RESOLUTION);
  
  ledcAttachPin(RGB_R_PIN, PWM_CHANNEL_R);
  ledcAttachPin(RGB_G_PIN, PWM_CHANNEL_G);
  ledcAttachPin(RGB_B_PIN, PWM_CHANNEL_B);
  
  setRGB(0, 0, 0);  // 初始全暗
  
  // 設定按鈕
  pinMode(BUTTON_PLAY, INPUT);
  
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
  Serial.println("按下黃色按鈕（GPIO 13）隨機抽籤播放音檔");
  Serial.println("========================================\n");
}

void loop() {
  // 讀取按鈕狀態
  bool buttonCurrent = (digitalRead(BUTTON_PLAY) == HIGH);
  
  // 偵測按鈕按下（從 LOW 變 HIGH）
  if (buttonCurrent == HIGH && lastButtonState == LOW) {
    // 觸發抽籤
    lottery();
    delay(50);  // 防彈跳
  }
  
  lastButtonState = buttonCurrent;
  
  // 減少 delay 時間，提高反應速度
  delay(5);
}
