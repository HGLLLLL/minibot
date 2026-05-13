#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- Pin Definitions ---
#define OLED_CLK    14
#define OLED_MOSI   13
#define OLED_RST    16
#define OLED_DC     17
#define OLED_CS     -1 

#define RTC_SDA     32
#define RTC_SCL     33

#define IMU_SDA     21
#define IMU_SCL     22

#define DFPLAYER_TX 26 
#define DFPLAYER_RX 27 
#define DFPLAYER_BUSY 25 // 重要：用來判斷音效是否播完 

#define BTN_L_PIN   4
#define BTN_R_PIN   5  
#define ENC_DT      19 
#define ENC_CLK     18 

// --- Music Tracks Definition (SD卡對應) ---
#define TRACK_BUTTON    1 // 0001.mp3: 按鈕音 
#define TRACK_ALARM     2 // 0002.mp3: 番茄鐘轉換鈴聲 
#define TRACK_MOZART    3 // 0003.mp3: 莫札特奏鳴曲 

enum AppMode { MODE_MENU, MODE_EYES, MODE_CLOCK, MODE_MUSIC, MODE_POMODORO, MODE_ANSWER };
enum PomoState {
    POMO_SETUP_WORK,
    POMO_SETUP_SHORT,
    POMO_SETUP_LONG,
    POMO_RUNNING,
    POMO_PAUSED,
    POMO_BREAK_RUNNING,
    POMO_BREAK_PAUSED,
    POMO_WAIT_ALARM  // <--- 必須要有這個
};

#endif