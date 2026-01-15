# SU-03T 燒錄步驟

## 硬體準備

### 1. CH340 接線

```
CH340 HW-597          →   SU-03T 腳位
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
TXD (發送)           →   B7 (RX 接收)
RXD (接收)           →   B6 (TX 發送)
5V                   →   I10 (電源)
GND                  →   I09 (地線)
```

**重要：TX 接 RX，RX 接 TX（交叉連接）**

### 2. 確認 CH340 驅動已安裝

**Windows：**
- 插入 CH340 後，裝置管理員中應顯示 COM 埠（如 COM3、COM4）
- 若無驅動，下載安裝：http://www.wch.cn/downloads/CH341SER_EXE.html

**Mac：**
- 下載驅動：https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver
- 重啟後插入 CH340，終端機執行 `ls /dev/tty.*` 應看到 `/dev/tty.wchusbserial*`

## 軟體燒錄步驟

### 方法 1：Windows 系統

1. **執行燒錄工具**
   - 開啟 `Hummingbird-M-Update-Tool` 資料夾
   - 執行 `UniOneUpdateTool.exe`

2. **設定連接參數**
   - 選擇正確的 **COM 埠**（裝置管理員中查看）
   - 波特率：**9600**
   - 資料位：**8**
   - 停止位：**1**
   - 校驗：**無**

3. **選擇音檔**
   - 點選「添加檔案」或「瀏覽」
   - 選擇你準備好的 WAV 檔案：
     - `audio/Dad_breakfast.wav`
     - `audio/SX_tabata.wav`
     - `audio/Mom_storytelling.wav`
   
4. **開始燒錄**
   - 確認 CH340 與 SU-03T 連接正確
   - 點擊「開始燒錄」或「更新」按鈕
   - 等待進度條完成（通常 1-3 分鐘）

5. **燒錄完成**
   - 看到「燒錄成功」提示
   - 拔除 CH340 連接
   - SU-03T 準備接回 ESP32

### 方法 2：Mac 系統（使用虛擬機）

#### 使用 Parallels Desktop 或 VMware Fusion

1. **設定 USB 轉發**
   - 啟動 Windows 虛擬機
   - 插入 CH340 USB
   - 在虛擬機選單中選擇「裝置」→「USB」→「CH340」
   - 將 USB 裝置連接到 Windows 虛擬機

2. **後續步驟同方法 1**

### 方法 3：Mac 系統（使用串口工具）

若你熟悉命令列，可以使用 Mac 原生的串口工具：

```bash
# 1. 確認串口裝置
ls /dev/tty.wchusbserial*

# 2. 使用 screen 或 minicom 連接（進階用法，需額外研究）
# 不建議新手使用
```

## 燒錄注意事項

### 音檔格式要求
- **格式**：WAV (PCM)
- **採樣率**：16000 Hz (16kHz)
- **聲道**：單聲道 (Mono)
- **位深度**：16-bit
- **編碼**：PCM signed 16-bit little-endian

### 轉換命令（已完成）
你的音檔已經轉換好了：
```bash
ffmpeg -i input.m4a -ar 16000 -ac 1 -sample_fmt s16 -acodec pcm_s16le output.wav
```

### 常見問題

**Q1: 找不到 COM 埠？**
- 確認 CH340 驅動已安裝
- 重新插拔 USB
- 更換 USB 埠

**Q2: 燒錄失敗？**
- 檢查接線是否正確（TX ↔ RX 交叉連接）
- 確認 5V 和 GND 連接穩固
- 確認 SU-03T 有供電（LED 應亮起）

**Q3: 燒錄成功但 ESP32 無法播放？**
- 檢查 ESP32 與 SU-03T 的 UART 接線
- 確認 ESP32 程式中的波特率為 9600
- 檢查發送的指令格式是否正確

**Q4: Mac 沒有 Windows 環境怎麼辦？**
- 找有 Windows 電腦的朋友幫忙燒錄
- 安裝 Parallels Desktop 試用版（14 天免費）
- 使用 Boot Camp 安裝 Windows（需要磁碟空間）

## 測試播放

燒錄完成後，將 SU-03T 接回 ESP32：

```cpp
// ESP32 測試程式碼
HardwareSerial ttsSerial(2);

void setup() {
  Serial.begin(115200);
  ttsSerial.begin(9600, SERIAL_8N1, 19, 18); // RX:19, TX:18
  
  delay(2000);
  
  // 播放音檔 1
  ttsSerial.println("[v1]");
  Serial.println("播放音檔 1");
  
  delay(5000);
  
  // 播放音檔 2
  ttsSerial.println("[v2]");
  Serial.println("播放音檔 2");
}

void loop() {
  // 空的
}
```

## 下一步

燒錄完成後，繼續開發計畫：
1. ✅ CH340 到貨
2. ⏳ 燒錄音檔至 SU-03T（正在進行）
3. ⏳ ESP32 測試播放
4. ⏳ 整合抽獎機制

---

**建立時間：** 2026-01-15
