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
#define DFPLAYER_BUSY 25 // Important: used to detect if sound finished playing 

#define BTN_L_PIN   4
#define BTN_R_PIN   5  
#define ENC_DT      19 
#define ENC_CLK     18 

// --- Music Tracks Definition (SD card mapping) ---
#define TRACK_BUTTON    1 // 0001.mp3: Button click sound 
#define TRACK_ALARM     2 // 0002.mp3: Pomodoro transition alarm 
#define TRACK_MOZART    3 // 0003.mp3: Mozart Sonata 

enum AppMode { MODE_MENU, MODE_EYES, MODE_CLOCK, MODE_MUSIC, MODE_POMODORO, MODE_ANSWER };
enum PomoState {
    POMO_SETUP_WORK,
    POMO_SETUP_SHORT,
    POMO_SETUP_LONG,
    POMO_RUNNING,
    POMO_PAUSED,
    POMO_BREAK_RUNNING,
    POMO_BREAK_PAUSED,
    POMO_WAIT_ALARM  // Required: wait for alarm sound to finish
};

#endif