#ifndef GLOBALS_H
#define GLOBALS_H

// ... (原本的 include 維持不變) ...
#include "Config.h"
#include <Wire.h>
#include <Adafruit_SH110X.h> 
#include <Adafruit_MPU6050.h>
#include <RTClib.h>
#include <DFRobotDFPlayerMini.h>
#include <OneButton.h>
#include <ESP32Encoder.h>
#include "FluxGarage_RoboEyes.h"
#include "esp_sleep.h"

// ... (外部變數宣告 維持不變) ...
extern TwoWire I2C_RTC;   
extern Adafruit_SH1106G display;
extern RoboEyes<Adafruit_SH1106G> roboEyes;
extern Adafruit_MPU6050 mpu;
extern RTC_DS3231 rtc;
extern DFRobotDFPlayerMini myDFPlayer;

extern OneButton btnL;
extern OneButton btnR;
extern ESP32Encoder encoder;

extern bool isAsleep;

extern int displayBrightness;
extern volatile bool isBrightnessChanged; // 新增這行
extern bool isTimeSetting;
extern bool wasMusicPlayingBeforeAlarm; // 新增：記錄響鈴前是否在聽歌

// System Data
extern AppMode currentMode;
extern volatile bool isUpdateRequired; 
extern DateTime currentDateTime;       
extern int currentMusicTrack;          
extern bool isMusicPlaying;            
extern bool isBotTired;    
extern bool isActionInProgress;            

// 番茄鐘變數
extern PomoState pomoState;
extern int pomoWorkTime;      
extern int pomoShortBreak;    
extern int pomoLongBreak;     
extern long pomoTimerSeconds; 
extern int pomoRoundCounter;  
extern int pomoTotalDuration; 

// 解答之書變數
// Answer Book variables
extern bool isAnswerRevealed;     
extern const char* currentAnswer; 
extern const char* const answers_pool[]; 
extern const int answers_count;   
extern const unsigned char image_book_bits[]; 

// Answer Book slot machine animation
extern bool isAnswerSpinning;
extern unsigned long answerSpinStartTime;
extern float answerSpinSpeed;
extern float answerSpinOffset;
extern int answerReelIndices[5];

// --- [新增] 音樂播放器狀態變數 ---
enum MusicSelection {
    SEL_PLAY,   // 播放鍵
    SEL_NEXT,   // 下一首
    SEL_VOL,    // 音量鍵
    SEL_PREV    // 上一首
};
extern const char* composerNames[];
extern const int totalTracks;

extern MusicSelection musicSelection; // 目前選中的按鈕
extern bool isVolumeAdjusting;        // 是否正在調整音量模式
extern int currentVolume;             // 當前音量 (0-30)

#endif