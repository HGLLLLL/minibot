#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Pin Definitions ---
// Display
#define OLED_CLK    14
#define OLED_MOSI   13
#define OLED_RST    16
#define OLED_DC     17
#define OLED_CS     -1 

// RTC
#define RTC_SDA     32
#define RTC_SCL     33

// IMU
#define IMU_SDA     21
#define IMU_SCL     22

// DFPlayer
#define DFPLAYER_TX 27 
#define DFPLAYER_RX 26 
#define DFPLAYER_BUSY 25

// Inputs
#define BTN_L_PIN   4  
#define BTN_R_PIN   5  
#define ENC_DT      19 
#define ENC_CLK     18 

// --- Music Tracks Definition ---
// 假設 SD 卡資料夾裡： 0001.mp3 是音樂, 0002.mp3 是番茄鐘鈴聲
#define TRACK_MUSIC     1
#define TRACK_ALARM     2 

// --- System States ---
enum AppMode {
    MODE_MENU,      // (預留)
    MODE_EYES,      // 陪伴
    MODE_CLOCK,     // 時鐘
    MODE_MUSIC,     // 音樂
    MODE_POMODORO,  // 番茄鐘
    MODE_ANSWER     // [新增] 解答之書
};

// --- [新增] 番茄鐘內部狀態 ---
enum PomoState {
    POMO_SETUP_WORK,   // 設定工作時間
    POMO_SETUP_SHORT,  // 設定短休息
    POMO_SETUP_LONG,   // 設定長休息
    POMO_RUNNING,      // 計時中
    POMO_PAUSED,       // 暫停中
    POMO_BREAK_RUNNING,// 休息計時中
    POMO_BREAK_PAUSED  // 休息暫停中
};

#endif