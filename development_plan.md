# 開發計畫

## 硬體規格

| 組件 | 型號/規格 | GPIO 腳位 | 說明 |
|------|-----------|-----------|------|
| ESP32 開發板 | DevKit V1 | - | 主控制器 |
| RGB LED 燈條 | Common Anode 5V | R:16, G:17, B:5 | 三色獨立 PWM 控制 |
| 藍牙喇叭 | Bose Mini II SoundLink | - | A2DP 藍牙連接 |
| 按鈕 1（黑） | 輕觸按鈕 | GPIO 2 | 抽獎觸發 |
| 按鈕 2（黃） | 輕觸按鈕 | GPIO 15 | 保留未用 |
| 按鈕 3（紅） | 輕觸按鈕 | GPIO 4 | 任務 1 |
| 按鈕 4（綠） | 輕觸按鈕 | GPIO 0 | 任務 2 |
| 按鈕 5（藍） | 輕觸按鈕 | GPIO 13 | 任務 3 |

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

### 藍牙音頻播放（ESP32 → 藍牙喇叭）

```cpp
#include "BluetoothA2DPSource.h"
#include <SPIFFS.h>

BluetoothA2DPSource a2dp_source;
File audioFile;

// 音頻回調函數
int32_t get_sound_data(Frame *frame, int32_t frame_count) {
  // 從 SPIFFS 讀取 WAV 音頻資料
  // 進行重採樣（16kHz → 44.1kHz）
  // 填充至藍牙音頻 frame
}

void setup() {
  SPIFFS.begin(true);
  a2dp_source.start("Bose Mini II SoundLink", get_sound_data);
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

### 3. 上傳至 ESP32 SPIFFS
將 WAV 檔案放入專案的 `data/` 資料夾，使用 PlatformIO 上傳：

```bash
pio run --target uploadfs
```

音檔會被上傳到 ESP32 的 SPIFFS 檔案系統，程式執行時從 SPIFFS 讀取並透過藍牙播放。

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

### 階段 4：藍牙音頻整合 ✅

**4-1 學習**
- ✅ ESP32-A2DP 函式庫研究
- ✅ 藍牙 A2DP Source 原理學習
- ✅ SPIFFS 檔案系統學習

**4-2 開發**
- ✅ SPIFFS 初始化與檔案系統掛載
- ✅ WAV 音檔上傳至 ESP32 SPIFFS
- ✅ 藍牙 A2DP Source 初始化
- ✅ 音頻回調函數實作
- ✅ 軟體重採樣功能（16kHz → 44.1kHz）
- ✅ 音頻緩衝區優化

**4-3 硬體準備**
- ✅ 藍牙喇叭配對（Bose Mini II SoundLink）
- ✅ 音檔格式轉換（m4a → WAV 16kHz）
- ✅ 準備 3 個音檔：
  - `audio/Dad_breakfast.wav`
  - `audio/SX_tabata.wav`
  - `audio/Mom_storytelling.wav`

**4-4 測試與產出**
- ✅ 藍牙連接穩定性測試
- ✅ 音頻播放品質驗證（成功解決採樣率不匹配問題）
- ✅ 整合至 `src/main.cpp`

---

### 階段 5：抽獎機制開發 📅

**5-1 開發**
- ⏳ 燈光秀結束後進入「可抽獎」狀態
- ⏳ 1 分鐘限時邏輯（`millis()` 計時）
- ⏳ 黑色按鈕觸發抽獎
- ⏳ 超時自動失效
- ⏳ 隨機選擇音檔播放（從 SPIFFS 讀取）
- ⏳ 透過藍牙喇叭播放音頻

**5-2 測試與產出**
- ⏳ 限時邏輯驗證
- ⏳ 隨機播放驗證
- ⏳ 藍牙播放穩定性測試
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
  - 藍牙連接失敗處理
  - SPIFFS 讀取失敗處理
  - 音檔播放失敗處理

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
- `SPIFFS.h`（檔案系統）
- `ESP32-A2DP`（藍牙音頻串流）

**工具**：
- ffmpeg（音頻格式轉換）
- PlatformIO（SPIFFS 檔案上傳）

---

## 相關文件

- [產品需求文件 (PRD)](PRD.md)
- [接線說明](wiring.md)
- [學習筆記](LEARN.md)
- [序列埠監視說明](序列埠監視說明.md)
