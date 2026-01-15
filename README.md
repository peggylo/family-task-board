# 家庭任務提醒機

ESP32 互動式任務提醒裝置，透過按鈕、RGB 燈光與語音回饋，讓完成任務變得有趣。

## 功能特色

- **三色按鈕控制**：紅/綠/藍按鈕對應三個任務，按下亮燈表示完成
- **燈光秀獎勵**：三燈全亮後觸發華麗彩虹燈光秀
- **抽籤機制**：燈光秀後 60 秒內按黃色按鈕，隨機播放獎勵語音
- **藍牙音頻**：透過藍牙喇叭播放預錄語音

## 硬體需求

- ESP32 DevKit V1
- RGB LED 燈條（共陽極）
- 5 個輕觸按鈕
- 藍牙喇叭（Bose Mini II SoundLink）

## 快速開始

```bash
# 使用 PlatformIO 編譯並上傳
pio run --target upload

# 上傳音檔到 SPIFFS
pio run --target uploadfs
```

## 文件

- [產品需求文件](doc/PRD.md) - 功能說明與使用流程
- [開發計畫](doc/development_plan.md) - 開發階段與技術架構
- [接線說明](doc/wiring.md) - 硬體接線圖與腳位配置
- [學習筆記](doc/LEARN.md) - 技術學習記錄
- [序列埠監視說明](doc/序列埠監視說明.md) - 除錯指南

## 專案結構

```
├── src/              # 主程式
├── test/             # 測試程式
├── data/             # SPIFFS 音檔
├── doc/              # 文件
└── components/       # 硬體元件照片與說明
```

---

# Family Task Reminder

An ESP32-based interactive task reminder device that makes completing tasks fun through buttons, RGB lights, and voice feedback.

## Features

- **Three-Color Button Control**: Red/Green/Blue buttons for three tasks - light up when completed
- **Light Show Reward**: Triggers a spectacular rainbow light show when all three lights are on
- **Lottery Mechanism**: Press yellow button within 60 seconds after light show to play random reward audio
- **Bluetooth Audio**: Plays pre-recorded voice messages through Bluetooth speaker

## Hardware Requirements

- ESP32 DevKit V1
- RGB LED Strip (Common Anode)
- 5 Tactile Buttons
- Bluetooth Speaker (Bose Mini II SoundLink)

## Quick Start

```bash
# Compile and upload using PlatformIO
pio run --target upload

# Upload audio files to SPIFFS
pio run --target uploadfs
```

## Documentation

- [Product Requirements](doc/PRD.md) - Feature description and user flow
- [Development Plan](doc/development_plan.md) - Development stages and technical architecture
- [Wiring Guide](doc/wiring.md) - Hardware wiring diagram and pin configuration
- [Learning Notes](doc/LEARN.md) - Technical learning records
- [Serial Monitor Guide](doc/序列埠監視說明.md) - Debugging guide

## Project Structure

```
├── src/              # Main program
├── test/             # Test programs
├── data/             # SPIFFS audio files
├── doc/              # Documentation
└── components/       # Hardware component photos and descriptions
```
