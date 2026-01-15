# 開發計畫

## 硬體規格

| 組件 | 型號/規格 | GPIO 腳位 | 說明 |
|------|-----------|-----------|------|
| ESP32 開發板 | DevKit V1 | - | 主控制器 |
| RGB LED 燈條 | Common Anode 5V | R:16, G:17, B:5 | 三色獨立 PWM 控制 |
| SU-03T 語音模組 | 離線語音播放 | TX:18, RX:19 | UART2 通訊 |
| 按鈕 1（黑） | 輕觸按鈕 | GPIO 2 | 抽獎觸發 |
| 按鈕 2（黃） | 輕觸按鈕 | GPIO 15 | 保留未用 |
| 按鈕 3（紅） | 輕觸按鈕 | GPIO 4 | 任務 1 |
| 按鈕 4（綠） | 輕觸按鈕 | GPIO 0 | 任務 2 |
| 按鈕 5（藍） | 輕觸按鈕 | GPIO 13 | 任務 3 |
| 喇叭 | 4Ω/8Ω | - | 接 SU-03T SPK+/SPK- |

詳細接線說明參考：[wiring.md](wiring.md)

---

## 軟體架構

### State Machine 狀態機

```
NORMAL 模式
  ├─ 偵測按鈕按下（紅/綠/藍）
  ├─ Toggle 對應燈光
  └─ 檢查三燈是否全亮
      └─ 是 → 進入 WAITING 模式

WAITING 模式（3 秒倒數）
  ├─ 燈光閃爍動畫
  ├─ 序列埠輸出倒數
  └─ 3 秒後 → 進入 LIGHT_SHOW 模式

LIGHT_SHOW 模式（10 秒燈光秀）
  ├─ 階段 1：快速閃爍（2.5 秒）
  ├─ 階段 2：彩虹漸變（2.5 秒）
  ├─ 階段 3：呼吸燈（2.5 秒）
  ├─ 階段 4：隨機閃爍（2.5 秒）
  └─ 結束後 → 全暗，重置狀態，進入 NORMAL 模式
```

### PWM 控制（Common Anode）

```cpp
// Common Anode 需要反轉邏輯
void setRGB(int red, int green, int blue) {
  ledcWrite(PWM_CHANNEL_R, 255 - red);   // 反轉
  ledcWrite(PWM_CHANNEL_G, 255 - green); // 反轉
  ledcWrite(PWM_CHANNEL_B, 255 - blue);  // 反轉
}
```

### UART 通訊（ESP32 ↔ SU-03T）

```cpp
HardwareSerial ttsSerial(2); // 使用 UART2

void setup() {
  ttsSerial.begin(9600, SERIAL_8N1, TTS_RX_PIN, TTS_TX_PIN);
}

void speakText(const char* text) {
  // 發送播放指令至 SU-03T
  ttsSerial.print("[");
  ttsSerial.print(text);
  ttsSerial.println("]");
}
```

---

## 音頻準備流程

### 1. 錄音
- 使用 iPhone 語音備忘錄或任何錄音 App
- 錄製 5-10 秒獎勵語音
- 環境安靜，語音清晰

### 2. 格式轉換
使用 ffmpeg 將 m4a 轉換為 WAV：

```bash
ffmpeg -i input.m4a -ar 16000 -ac 1 -sample_fmt s16 -acodec pcm_s16le output.wav
```

**參數說明**：
- `-ar 16000`：採樣率 16kHz
- `-ac 1`：單聲道
- `-sample_fmt s16`：16-bit 格式
- `-acodec pcm_s16le`：PCM 編碼

### 3. 燒錄至 SU-03T
1. 連接 CH340 燒錄器至 SU-03T：
   - CH340 TX → SU-03T B7 (RX)
   - CH340 RX → SU-03T B6 (TX)
   - CH340 5V → SU-03T I10
   - CH340 GND → SU-03T I09

2. 使用「串口燒錄軟體」（從官方下載）

3. 選擇 WAV 檔案並燒錄至 Flash

4. 燒錄完成後，ESP32 即可透過 UART 發送指令播放

---

## 開發階段

### 階段 0：環境準備與基礎學習 ✅

**0-1 硬體準備**
- ✅ 整理電子材料清單
- ✅ 認識麵包板與接線
- ✅ 產出：`components/` 資料夾（元件照片與規格）
- ✅ 產出：`components/ESP32_麵包板孔位對應.md`

**0-2 開發環境**
- ✅ 建立 PlatformIO 專案
- ✅ ESP32 GPIO 基礎學習
- ✅ 單 LED 閃爍程式
- ✅ 雙 LED 閃爍程式

**0-3 測試與產出**
- ✅ 功能驗證
- ✅ 產出：`test/test_single_led_blink.cpp`
- ✅ 產出：`test/test_dual_led_blink.cpp`

---

### 階段 1：按鈕輸入控制 ✅

**1-1 開發**
- ✅ 5 個按鈕讀取功能
- ✅ 按鈕防彈跳處理
- ✅ 按鈕控制單色 LED

**1-2 測試與產出**
- ✅ 按鈕功能驗證
- ✅ 產出：`test/test_five_buttons_single_led.cpp`

---

### 階段 2：RGB LED 與 Toggle 功能 ✅

**2-1 開發**
- ✅ PWM 設定（頻率 5000Hz，解析度 8-bit）
- ✅ 實作 `setRGB()` 函數
- ✅ Common Anode 反轉邏輯
- ✅ RGB 三色獨立控制
- ✅ Toggle 開關邏輯（按一次亮、再按一次滅）
- ✅ 三顏色按鈕獨立狀態管理

**2-2 測試與產出**
- ✅ RGB 顏色顯示驗證
- ✅ Toggle 功能驗證
- ✅ 整合至 `src/main.cpp`

---

### 階段 3：燈光秀實作 ✅

**3-1 開發**
- ✅ State Machine 架構（NORMAL / WAITING / LIGHT_SHOW）
- ✅ 三燈全亮偵測邏輯
- ✅ `millis()` 非阻塞計時
- ✅ 3 秒倒數閃爍動畫
- ✅ 10 秒四階段燈光秀：
  - 快速閃爍（2.5 秒）
  - 彩虹漸變（2.5 秒）
  - 呼吸燈（2.5 秒）
  - 隨機閃爍（2.5 秒）
- ✅ 燈光秀結束後全暗並重置狀態

**3-2 測試與產出**
- ✅ 完整流程驗證
- ✅ 產出：`test/test_rgb_lightshow.cpp`
- ✅ 整合至 `src/main.cpp`

---

### 階段 4：SU-03T 音頻模組整合 ⏳

**4-1 學習**
- ✅ SU-03T 官方規格書研讀
- ✅ 產出：`components/SU03T.md`

**4-2 開發**
- ✅ UART2 初始化（9600 baud）
- ✅ `speakText()` 函數實作
- ✅ 黑色按鈕觸發測試播放

**4-3 硬體準備**
- ✅ SU-03T 接線完成
- ✅ 喇叭連接
- ✅ 音檔格式轉換（m4a → WAV 16kHz）
- ✅ 準備 3 個音檔：
  - `audio/Dad_breakfast.wav`
  - `audio/SX_tabata.wav`
  - `audio/Mom_storytelling.wav`

**4-4 待完成**
- 🛒 購買 CH340 燒錄器（等待送達）
- ⏳ 下載 SU-03T 燒錄軟體
- ⏳ 燒錄音檔至 SU-03T Flash
- ⏳ 測試 ESP32 播放指令
- ⏳ 實際播放效果驗證

**4-5 測試與產出**
- ⏳ UART 通訊穩定性測試
- ⏳ 整合至 `src/main.cpp`

---

### 階段 5：抽獎機制開發 📅

**5-1 開發**
- ⏳ 燈光秀結束後進入「可抽獎」狀態
- ⏳ 1 分鐘限時邏輯（`millis()` 計時）
- ⏳ 黑色按鈕觸發抽獎
- ⏳ 超時自動失效
- ⏳ 隨機選擇 1-3 號音檔播放（`random()`）

**5-2 測試與產出**
- ⏳ 限時邏輯驗證
- ⏳ 隨機播放驗證
- ⏳ 整合至 `src/main.cpp`

---

### 階段 6：完整流程整合 📅

**6-1 開發**
- ⏳ 整合所有功能至單一流程
- ⏳ 邊界條件處理：
  - 燈光秀期間按按鈕
  - 抽獎超時處理
  - 重複抽獎防止
- ⏳ 錯誤處理：
  - UART 通訊失敗
  - 音檔播放失敗

**6-2 測試與產出**
- ⏳ 完整使用者流程測試
- ⏳ 長時間穩定性測試
- ⏳ 程式碼重構與註解完善
- ⏳ 最終版 `src/main.cpp`

---

### 階段 7：外觀設計與組裝 📅

**7-1 開發**
- ⏳ 外殼/展示面板設計
- ⏳ 固定電路（麵包板 or 洞洞板）
- ⏳ 美化裝飾

**7-2 測試與產出**
- ⏳ 使用者測試與回饋收集
- ⏳ 最終成品

---

## 技術棧

**開發環境**：
- PlatformIO
- ESP32 Arduino Framework

**主要程式庫**：
- `Arduino.h`（內建）
- `HardwareSerial.h`（UART 通訊）

**工具**：
- ffmpeg（音頻格式轉換）
- CH340 燒錄軟體（SU-03T 音檔燒錄）

---

## 相關文件

- [產品需求文件 (PRD)](PRD.md)
- [接線說明](wiring.md)
- [學習筆記](LEARN.md)
- [序列埠監視說明](序列埠監視說明.md)
