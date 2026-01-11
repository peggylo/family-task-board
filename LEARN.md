# ESP32 開發學習筆記

學習日期：2026-01-11

本文件記錄了從零開始建立 ESP32 開發環境，以及理解嵌入式開發的核心概念。

---

## 目錄

1. [開發環境設置](#開發環境設置)
2. [序列埠通訊概念](#序列埠通訊概念)
3. [專案檔案結構](#專案檔案結構)
4. [模組化開發概念](#模組化開發概念)
5. [C++ 標頭檔和實作檔](#cpp-標頭檔和實作檔)
6. [編譯和連結過程](#編譯和連結過程)
7. [函式庫使用](#函式庫使用)
8. [Arduino 框架概念](#arduino-框架概念)
9. [實用指令](#實用指令)

---

## 開發環境設置

### 成功安裝的項目

- ✅ PlatformIO Core (v6.1.18)
- ✅ ESP32 開發平台 (espressif32)
- ✅ Arduino 框架
- ✅ ESP32 工具鏈（toolchain-xtensa-esp32）

### 測試結果

- 開發板：ESP32-D0WD-V3 (revision v3.0)
- MAC 地址：24:d7:eb:19:bc:00
- 序列埠：/dev/cu.usbserial-1120
- 測試程式：LED 閃爍控制 ✅ 成功

---

## 序列埠通訊概念

### 什麼是序列通訊？

ESP32 透過 USB 線和電腦「對話」，傳遞訊息的方式。

### 波特率（Baud Rate）

**波特率 = 通訊速度（每秒傳輸的位元數）**

**比喻：**兩個人用對講機通話的速度
- 9600 波特率 = 每秒說 9600 個字
- 115200 波特率 = 每秒說 115200 個字（更快）

**關鍵原則：雙方必須用相同速度才能聽懂**

### ESP32 開機的兩個階段

```
階段 1：Bootloader（系統載入程式）
├── 使用 74880 波特率
├── 輸出硬體初始化資訊
└── 無法修改（出廠內建）

階段 2：你的程式（main.cpp）
├── 使用你設定的波特率（通常 115200）
├── 在 Serial.begin(115200) 設定
└── 可以自由修改
```

### 為什麼開機時會看到亂碼？

```
監視器設定：115200
Bootloader 輸出：74880
→ 速度不匹配 = 亂碼（ets@ets@...）

等程式啟動後：
監視器設定：115200
程式輸出：115200
→ 速度匹配 = 正常顯示
```

**比喻：**朋友用台語快速講（74880），你用國語速度聽（115200），當然聽不懂。

---

## 專案檔案結構

### 完整結構

```
提醒機/
├── src/               【必要】你的程式碼
│   └── main.cpp
├── include/           【可選】全域標頭檔（像 config）
├── lib/               【可選】自訂函式庫
├── test/              【可選】測試程式
├── .pio/              【自動】編譯輸出和下載的工具
├── .vscode/           【建議】VSCode 設定
└── platformio.ini     【必要】專案設定檔
```

### 比喻：工廠結構

```
platformio.ini = 工廠營運手冊（設定）
src/ = 生產車間（實際工作）
include/ = 設計圖紙櫃（全域定義）
lib/ = 原料倉庫（可重用的模組）
test/ = 品質檢驗室（測試）
.pio/ = 生產設備和成品倉庫（自動管理）
.vscode/ = 辦公室管理系統（編輯器設定）
```

### 各目錄用途

#### platformio.ini - 專案設定

```ini
[env:esp32dev]
platform = espressif32      # 硬體平台
board = esp32dev            # 開發板型號
framework = arduino         # 程式框架
monitor_speed = 115200      # 序列埠波特率
```

#### src/ - 程式碼

存放你的主程式和其他 .cpp 檔案。

#### include/ - 全域標頭檔

存放所有檔案都會用到的定義，例如：

```cpp
// include/config.h
#define LED_PIN 5
#define WIFI_SSID "MyWiFi"
#define MAX_RETRY 3
```

**本質上就是 config 的概念。**

#### lib/ - 自訂函式庫

存放可重複使用的程式模組。

```
lib/
├── LED/
│   ├── LED.h
│   └── LED.cpp
└── Sensor/
    ├── Sensor.h
    └── Sensor.cpp
```

#### .pio/ - 自動產生的檔案

```
.pio/
├── build/              編譯輸出
│   └── esp32dev/
│       ├── firmware.bin    最終燒錄檔
│       └── *.o             中間檔案
└── libdeps/            第三方函式庫
    └── esp32dev/
        ├── FastLED/
        └── WiFi/
```

**可以隨時刪除，下次編譯會重建。**

---

## 模組化開發概念

### 為什麼要模組化？

#### 小專案（< 200 行）

```
main.cpp（全寫在一起）
✅ 簡單直接
✅ 容易理解
```

#### 大專案（> 500 行）

```
main.cpp（300 行）
- LED 控制（50 行）
- 按鈕讀取（40 行）
- WiFi 連線（80 行）
- 感測器（60 行）
- 資料上傳（70 行）

→ 捲來捲去找不到東西！
```

#### 模組化後

```
lib/LED/          LED 功能獨立
lib/Button/       按鈕功能獨立
lib/WiFi/         WiFi 功能獨立
lib/Sensor/       感測器功能獨立

main.cpp（30 行）統籌調度
```

### 模組化的好處

1. **可讀性**：每個模組專注一件事
2. **可重用**：下次專案可以直接複製整個模組
3. **可維護**：修改不會影響其他部分
4. **可分享**：別人可以使用你的模組

### 何時應該模組化？

| 專案規模 | 行數 | 建議 |
|---------|------|------|
| 小專案 | < 100 | 全寫在 main.cpp |
| 中專案 | 100-500 | 考慮分檔到 src/ |
| 大專案 | > 500 | 使用 lib/ 模組化 |

**初學建議：**等程式超過 200 行，開始覺得亂了再考慮模組化。

---

## C++ 標頭檔和實作檔

### 為什麼 C++ 要分 .h 和 .cpp？

這是 C++ 編譯語言的特性。

#### Python（直譯式）

```python
# 邊讀邊執行
print("Hello")    # 讀這行 → 執行
x = 5             # 讀這行 → 執行
```

就像：看食譜一邊做菜

#### C++（編譯式）

```
1. 先讀完整份程式碼
2. 檢查語法、確認所有功能都存在
3. 轉換成機器碼
4. 最後才執行

就像：讀完食譜、確認材料、規劃流程，然後才開始做
```

### .h（標頭檔）和 .cpp（實作檔）

#### 比喻：餐廳菜單

```
.h（標頭檔）= 菜單
- 只列菜名和價格
- 宮保雞丁 $120
- 不說怎麼做

.cpp（實作檔）= 廚房食譜
- 實際做法
- 雞肉切丁、加花椒、快炒...
```

#### 實際範例

**led.h（宣告）：**
```cpp
#ifndef LED_H
#define LED_H

void led_on();      // 告訴別人「我有這個功能」
void led_off();     // 但不說怎麼做

#endif
```

**led.cpp（實作）：**
```cpp
#include <Arduino.h>
#include "led.h"

void led_on() {                  // 這裡才寫實際做法
  digitalWrite(5, HIGH);
}

void led_off() {
  digitalWrite(5, LOW);
}
```

**main.cpp（使用）：**
```cpp
#include "led.h"    // 引入宣告

void setup() {
  pinMode(5, OUTPUT);
}

void loop() {
  led_on();         // 直接使用！
  delay(1000);
  led_off();
  delay(1000);
}
```

### 為什麼這樣設計？

**好處：**
1. **編譯更快**：編譯器看 .h 就知道有什麼功能，可以平行處理
2. **模組化**：介面（.h）和實作（.cpp）分離
3. **換實作不影響使用者**：只要 .h 不變，改 .cpp 不影響其他程式

### main.cpp 需要 main.h 嗎？

**不需要！**

**規則：只有「被別人使用」的程式才需要 .h**

```
main.cpp = 程式入口
→ 沒有人會「使用」main.cpp
→ 不需要 main.h

led.cpp = 被 main.cpp 使用
→ 需要 led.h 告訴 main.cpp 有什麼功能
```

---

## 編譯和連結過程

### 機器碼是什麼？

**機器碼 = CPU 能直接執行的指令（0101010...）**

### 所有 C++ 都需要編譯

不只是硬體！

| 裝置 | CPU | 需要編譯 |
|------|-----|---------|
| ESP32 | Xtensa 32-bit | ✅ |
| 你的電腦 | Intel x86 / ARM | ✅ |
| 手機 | ARM | ✅ |
| Chrome 瀏覽器 | 對應平台 | ✅ |

### 編譯過程

```
步驟 1：預處理
- 處理 #include（把標頭檔內容貼進來）
- 處理 #define

步驟 2：編譯
- main.cpp → main.cpp.o（機器碼）
- led.cpp → led.cpp.o（機器碼）

步驟 3：連結（Linking）
- 把所有 .o 檔組合起來
- main.cpp.o + led.cpp.o → firmware.elf

步驟 4：產生最終檔案
- firmware.elf → firmware.bin（燒錄檔）
```

### 比喻：蓋房子

```
原始碼（.cpp） = 設計圖
編譯 = 把設計圖變成建材
連結 = 把建材組裝成房子
機器碼（.bin） = 完成的房子
```

---

## 函式庫使用

### 什麼是函式庫？

**函式庫 = 別人寫好的工具箱**

就像蓋房子不用自己做磚頭，直接買現成的。

### 第三方函式庫的位置

**不是放在你的專案 include/ 裡！**

```
提醒機/
├── include/           ← 你自己寫的標頭檔
├── lib/               ← 你自己寫的函式庫
└── .pio/
    └── libdeps/       ← 第三方函式庫在這裡！
        └── esp32dev/
            ├── FastLED/
            ├── WiFi/
            └── ...
```

### #include 的作用

**#include = 把標頭檔內容複製到這裡**

#### 範例

**你的 main.cpp：**
```cpp
#include <FastLED.h>

void setup() {
  FastLED.addLeds(...);
}
```

**編譯器處理後：**
```cpp
// ===== FastLED.h 的內容被貼進來 =====
class CFastLED {
  public:
    void addLeds(...);    // 現在知道有這個功能了
    void show();
};
// ===== 結束 =====

void setup() {
  FastLED.addLeds(...);   // 編譯器知道這是什麼了！
}
```

### 只複製宣告，不複製實作

```
#include <FastLED.h>
→ 只複製 FastLED.h（宣告，約 500 行）

FastLED.cpp（實作，約 5000 行）
→ 不會被複製
→ 在連結階段才組合進來
```

### 比喻：點外賣

```
#include = 看菜單（知道有什麼菜）
實際函式庫 = 餐廳廚房（做菜的地方）

流程：
1. 看菜單（#include）→ 知道有宮保雞丁
2. 點菜（呼叫 FastLED.addLeds()）
3. 編譯連結 = 廚房做好菜送來
```

---

## Arduino 框架概念

### 層次關係

```
┌─────────────────────────────────────┐
│  你的程式 (main.cpp)                │  ← 你寫的程式碼
│  使用 digitalWrite(), Serial 等     │
├─────────────────────────────────────┤
│  Arduino Framework                  │  ← 簡單易用的函式
│  把複雜操作封裝成簡單指令            │
├─────────────────────────────────────┤
│  Espressif32 Platform               │  ← 編譯器和工具
│  ESP32 專用的開發工具鏈              │
├─────────────────────────────────────┤
│  ESP32 硬體 (board: esp32dev)       │  ← 實體晶片
│  CPU、RAM、GPIO 等                  │
└─────────────────────────────────────┘
```

### platformio.ini 解析

```ini
[env:esp32dev]              # 環境設定名稱
platform = espressif32      # 硬體平台（工具鏈）
board = esp32dev            # 開發板型號（硬體）
framework = arduino         # 程式框架（寫法）
```

**翻譯：**在 ESP32 開發板上，使用 Espressif 的工具，用 Arduino 的簡單寫法來寫程式。

### Platform、Board、Framework 的關係

**比喻：蓋房子**

```
Board（開發板）= 土地
Platform（平台）= 地基和建材供應商
Framework（框架）= 建築工法
你的程式 = 房子內部裝潢
```

### Arduino Framework vs ESP-IDF

ESP32 可以用不同的框架開發：

#### Arduino Framework（你現在用的）

```cpp
void setup() {
  Serial.begin(115200);
  pinMode(5, OUTPUT);
}

void loop() {
  digitalWrite(5, HIGH);
  delay(1000);
}
```

- ✅ 簡單易學
- ✅ 函式庫多
- ✅ 社群資源豐富

#### ESP-IDF（原生框架）

```c
#include "driver/gpio.h"

void app_main() {
  gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
  while(1) {
    gpio_set_level(GPIO_NUM_5, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
```

- ✅ 功能更強大
- ❌ 學習曲線陡峭

### Arduino 是硬體還是軟體？

**Arduino 原本是硬體（Arduino Uno 等開發板）**

**但 Arduino Framework 是一種「程式寫法」：**
- 提供 setup() 和 loop() 結構
- 提供簡單的函式（pinMode, digitalWrite）
- 被移植到很多不同硬體

**所以：**
- ESP32 可以用 Arduino 框架 ✅
- STM32 可以用 Arduino 框架 ✅
- Raspberry Pi Pico 可以用 Arduino 框架 ✅

**硬體和框架是分開的！**

---

## .ino vs .cpp

### Arduino IDE 使用 .ino

**自動魔法：**

你寫的 `Blink.ino`：
```cpp
void setup() {
  pinMode(5, OUTPUT);
}

void loop() {
  digitalWrite(5, HIGH);
  delay(1000);
}
```

Arduino 自動轉換成：
```cpp
#include <Arduino.h>    // 自動加這行

void setup() {
  pinMode(5, OUTPUT);
}

void loop() {
  digitalWrite(5, HIGH);
  delay(1000);
}
```

**就這樣！只是自動加 #include**

### PlatformIO 使用 .cpp

**沒有自動魔法，要自己寫：**

```cpp
#include <Arduino.h>    // 要自己寫

void setup() {
  pinMode(5, OUTPUT);
}

void loop() {
  digitalWrite(5, HIGH);
  delay(1000);
}
```

### 模組化都要自己寫

**重要：不論 .ino 還是 .cpp，模組化的 .cpp 和 .h 都要自己手動建立！**

```
Arduino IDE 專案：
├── Blink.ino         ← 自動加 #include
├── led.cpp           ← 你自己寫
└── led.h             ← 你自己寫

PlatformIO 專案：
├── main.cpp          ← 要自己寫 #include
├── led.cpp           ← 你自己寫
└── led.h             ← 你自己寫
```

---

## 副檔名概念

**副檔名 = 檔案的身分證**

告訴電腦和人類這是什麼類型的檔案。

| 副檔名 | 意思 | 用途 |
|--------|------|------|
| **.cpp** | C++ 原始碼 | 程式實作 |
| **.h** | Header 標頭 | 程式宣告 |
| **.ino** | Arduino | Arduino 程式（其實就是 .cpp） |
| **.py** | Python | Python 程式 |
| **.js** | JavaScript | JavaScript 程式 |
| **.md** | Markdown | 文件 |

---

## 實用指令

### 編譯和上傳

```bash
# 只編譯
pio run

# 編譯並上傳到 ESP32
pio run --target upload

# 清除編譯輸出
pio run --target clean
```

### 序列埠監視

```bash
# 開啟序列埠監視器（使用 platformio.ini 設定的波特率）
pio device monitor

# 指定波特率
pio device monitor --baud 115200

# 列出可用的序列埠
ls /dev/cu.*
```

### 專案管理

```bash
# 初始化新專案
pio project init --board esp32dev

# 安裝函式庫
pio lib install "FastLED"

# 列出已安裝的函式庫
pio lib list
```

---

## 關鍵觀念總結

### 1. 波特率

- 通訊速度，雙方必須一致
- ESP32 bootloader 用 74880（固定）
- 你的程式可以自訂（通常 115200）
- 開機亂碼是正常的

### 2. 專案結構

- **src/** - 你的程式（必要）
- **platformio.ini** - 設定檔（必要）
- **include/** - 全域標頭（可選，像 config）
- **lib/** - 自訂模組（可選）
- **.pio/** - 自動產生（可刪除）

### 3. 模組化

- 小專案（< 200 行）：不用模組化
- 大專案：使用 lib/ 分模組
- .cpp 和 .h 要自己手動建立
- .ino 不會自動產生模組

### 4. 標頭檔

- .h = 宣告（菜單）
- .cpp = 實作（食譜）
- main.cpp 不需要 main.h（沒人用它）
- 只有被別人用的程式才需要 .h

### 5. 編譯

- 所有 C++ 都要編譯（不只硬體）
- 編譯 = 翻譯成機器碼
- 連結 = 組合所有模組
- 比 Python 快很多

### 6. 函式庫

- #include = 複製宣告
- 實作在連結時才加入
- 第三方函式庫在 .pio/libdeps/
- 你的函式庫在 lib/

### 7. Arduino 框架

- Arduino = 一種程式寫法
- 不是硬體專屬
- ESP32 用 Arduino 框架 = 用簡單的寫法
- Platform ≠ Framework ≠ Board

### 8. .ino vs .cpp

- .ino 自動加 #include
- .cpp 要自己寫 #include
- 模組化都要自己寫
- 只是主檔案的小差異

---

## 個人心得與反思

### .h 檔在 AI 時代的意義：類似 Snapshot 的概念

在學習 C++ 的標頭檔（.h）和實作檔（.cpp）分離時，發現這個設計理念在 AI 協作時代有了新的意義。

#### 核心相似性

**.h 檔的本質：**
- 只包含介面資訊（函式簽名、類別定義）
- 不含完整實作細節
- 告訴編譯器「有什麼可用」即可
- 讓編譯器快速檢查語法

**AI 時代的 Snapshot：**
- 只包含關鍵資訊（函式簽名、類別結構）
- 不需要完整程式碼
- 讓 AI 知道「有什麼可用」就能理解
- 加速 AI 理解和推理

#### 共通核心：最小必要資訊

```
編譯器思維（傳統）：
「我只需要知道 led_on() 存在，不用知道實作細節」
→ 看 .h 就夠了
→ 實作在連結時再處理

AI 思維（現代）：
「我只需要知道有 LED 類別和這些方法，不用看全部實作」
→ 看 snapshot 就夠了
→ 需要時再深入查看細節
```

#### 實務價值

在 AI 協作時代，「宣告與實作分離」的設計變得更有價值：

1. **快速理解**：AI 從 snapshot 快速掌握專案結構
2. **節省 token**：不需讀取完整實作就能討論架構
3. **模組化思維**：清楚的介面定義讓 AI 更容易協助開發

**C++ 幾十年前的設計，在 AI 協作時代意外地實用！**

---

### .ino 自動魔法在 AI 時代的價值重估

Arduino IDE 的 .ino 檔會自動加入 `#include <Arduino.h>`，這個「魔法」在 AI 時代的優勢大幅改變。

#### 傳統優勢（沒有 AI）

```cpp
// 初學者寫 .ino
void setup() {
  pinMode(5, OUTPUT);  // 不用知道從哪來
}
```

**優勢：**
- 降低學習門檻
- 不用理解 #include 概念
- 專注在功能而非結構

#### AI 時代的改變

```cpp
// AI 幫你寫 .cpp
#include <Arduino.h>  // AI 輕鬆加這行

void setup() {
  pinMode(5, OUTPUT);
}
```

**關鍵發現：AI 寫這行根本不費力！**

#### 優劣勢對比

| 考量點 | .ino 優勢 | AI 時代影響 |
|--------|----------|------------|
| 自動 include | ✅ 方便 | ⚡ AI 輕鬆處理 |
| 專案結構清晰 | ❌ 不如 .cpp | ✅ .cpp 更明確 |
| 工具整合 | ❌ Arduino IDE 限定 | ✅ .cpp 通用 |
| 模組化開發 | ❌ 都要自己寫 | ⚡ 沒差別 |
| 版本控制 | ❌ 格式特殊 | ✅ .cpp 標準 |
| 依賴關係 | ❌ 隱藏 | ✅ .cpp 明確 |

#### 結論

**在 AI 協作的現代開發環境：**

1. **.ino 的魔法優勢大幅降低**
   - AI 可以自動處理 #include
   - 明確的 .cpp 反而更容易理解

2. **.cpp 的「劣勢」變成優勢**
   - 明確顯示依賴關係
   - AI 更容易理解專案結構
   - 更好的工具整合

3. **PlatformIO + AI 的組合更適合現代開發**
   - 完整的專案結構
   - 明確的依賴管理
   - AI 處理繁瑣細節

**Arduino IDE 的自動魔法，在 AI 時代確實變得不那麼重要了。**

---

**持續更新中...**
