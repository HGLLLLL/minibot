#ifndef GLOBALS_H
#define GLOBALS_H

// ... (original includes unchanged) ...
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

// ... (external variable declarations unchanged) ...
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
extern volatile bool isBrightnessChanged; // Brightness change flag
extern bool isTimeSetting;
extern bool wasMusicPlayingBeforeAlarm; // Whether music was playing before alarm

// System Data
extern AppMode currentMode;
extern volatile bool isUpdateRequired; 
extern DateTime currentDateTime;       
extern int currentMusicTrack;          
extern bool isMusicPlaying;            
extern bool isBotTired;    
extern bool isActionInProgress;            

// Pomodoro timer variables
extern PomoState pomoState;
extern int pomoWorkTime;      
extern int pomoShortBreak;    
extern int pomoLongBreak;     
extern long pomoTimerSeconds; 
extern int pomoRoundCounter;  
extern int pomoTotalDuration; 

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

// --- Music player state variables ---
enum MusicSelection {
    SEL_PLAY,   // Play button
    SEL_NEXT,   // Next track
    SEL_VOL,    // Volume
    SEL_PREV    // Previous track
};
extern const char* const composerNames[];
extern const int totalTracks;

extern MusicSelection musicSelection; // Currently selected button
extern bool isVolumeAdjusting;        // Whether volume adjust mode is active
extern int currentVolume;             // Current volume (0-30)

#endif