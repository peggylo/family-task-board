# 開發計畫

## 硬體規格

| 組件 | 型號/規格 | GPIO 腳位 | 說明 |
|------|-----------|-----------|------|
| ESP32 開發板 | DevKit V1 | - | 主控制器 |
| RGB LED 燈條 | Common Anode 5V | R:16, G:17, B:5 | 三色獨立 PWM 控制 |
| 藍牙喇叭 | Bose Mini II SoundLink | - | A2DP 藍牙連接 |
| 按鈕 1（黃） | 輕觸按鈕 | GPIO 13 | 抽獎觸發 |
| 按鈕 2（黑） | 輕觸按鈕 | GPIO 14 | 保留未用 |
| 按鈕 3（紅） | 輕觸按鈕 | GPIO 12 | 控制紅燈 |
| 按鈕 4（綠） | 輕觸按鈕 | GPIO 33 | 控制綠燈 |
| 按鈕 5（藍） | 輕觸按鈕 | GPIO 32 | 控制藍燈 |

詳細接線說明參考：[wiring.md](wiring.md)

---

## 軟體架構

### State Machine 狀態機

```
NORMAL 模式
  ├─ 偵測按鈕按下（紅/綠/藍）
  ├─ Toggle 對應燈光
  └─ 檢查三燈是否全亮
      └─ 是 → 直接進入 LOTTERY 模式

LOTTERY 模式（60 秒限時抽獎）
  ├─ 持續播放彩虹燈光秀（8秒循環）
  │   ├─ 階段 1：彩虹循環（0-3 秒）
  │   ├─ 階段 2：快速彩虹（3-6 秒）
  │   └─ 階段 3：呼吸燈彩虹（6-8 秒）
  ├─ 每 10 秒顯示剩餘時間
  ├─ 偵測黃色按鈕按下
  │   └─ 是 → 抽籤並播放音檔
  ├─ 檢查是否超時（60 秒）
  │   └─ 是 → 重置所有燈光與狀態
  └─ 播放完成或超時 → 全暗，重置狀態，進入 NORMAL 模式
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

### 階段 5：抽獎機制開發 ✅

**5-1 開發**
- ✅ 三燈全亮後進入「可抽獎」狀態（LOTTERY 模式）
- ✅ 1 分鐘限時邏輯（`millis()` 計時，LOTTERY_TIMEOUT）
- ✅ 黃色按鈕觸發抽獎（BUTTON_1）
- ✅ 超時自動失效並重置所有狀態
- ✅ 隨機選擇音檔播放（`selectAudioFile()` 函數，支援 Dad/Mom/SX 三類別）
- ✅ 透過藍牙喇叭播放音頻（A2DP Source）
- ✅ 抽獎期間持續播放彩虹燈光秀（8秒循環）

**5-2 測試與產出**
- ✅ 限時邏輯驗證（60秒倒數，每10秒顯示剩餘時間）
- ✅ 隨機播放驗證（三類音檔隨機抽選）
- ✅ 藍牙播放穩定性測試
- ✅ 整合至 `src/main.cpp`

---

### 階段 6：完整流程整合 ✅

**6-1 開發**
- ✅ 整合所有功能至單一流程（NORMAL ↔ LOTTERY 狀態機）
- ✅ 邊界條件處理：
  - ✅ 抽獎期間按鈕邏輯（NORMAL 模式下才處理紅綠藍按鈕）
  - ✅ 抽獎超時處理（60秒後自動重置）
  - ✅ 重複抽獎防止（`lotteryUsed` 標記）
- ✅ 錯誤處理：
  - ✅ 藍牙連接失敗處理（狀態回調與 LED 指示）
  - ✅ SPIFFS 讀取失敗處理（初始化檢查與音檔掃描）
  - ✅ 音檔播放失敗處理（檔案開啟檢查與播放逾時保護）

**6-2 測試與產出**
- ✅ 完整使用者流程測試（按鈕 → 三燈全亮 → 抽獎 → 播放 → 重置）
- ✅ 程式碼重構與註解完善（繁體中文註解）
- ✅ 最終版 `src/main.cpp`（含音檔分類、重採樣、狀態管理）

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
