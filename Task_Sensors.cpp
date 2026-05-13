#include "Globals.h"
#include "HardwareSerial.h"
#include <Preferences.h>

// Create Preferences object for persistent storage across power cycles
Preferences prefs;

// Task function declaration
void TaskSensorsCode(void * parameter);

HardwareSerial mySerial(2); 

// --- Internal control variables ---
long oldEncPos = 0;
unsigned long lastRtcUpdate = 0;
unsigned long lastInteractionTime = 0; 
unsigned long actionStartTime = 0;
unsigned long lastPomoTick = 0; 

// --- Settings persistence variables ---
unsigned long lastSettingChangeTime = 0; 
bool settingsNeedSave = false;

// bool isTimeSetting = false; // Time-setting mode flag

// Temp variables for pomodoro "wait for alarm to finish" logic
PomoState pendingNextState; 
long pendingNextTimer = 0;
unsigned long alarmWaitStartTime = 0;

// Function declarations
void checkSensorsAndCommandEyes();

// --- Button callback functions ---

void handleBtnLClick() {
    lastInteractionTime = millis();

    // [Pomodoro] Setup phase: go back one step
    if (currentMode == MODE_POMODORO) {
        if (pomoState == POMO_SETUP_SHORT) { pomoState = POMO_SETUP_WORK; isUpdateRequired = true; return; }
        if (pomoState == POMO_SETUP_LONG)  { pomoState = POMO_SETUP_SHORT; isUpdateRequired = true; return; }
        if (pomoState != POMO_SETUP_WORK) { return; } 
    }

    // [Answer Book] Return from revealed state
    if (currentMode == MODE_ANSWER) {
        if (isAnswerRevealed) {          // Only act when answer is displayed
            isAnswerRevealed = false; 
            isUpdateRequired = true;
            return;                      // Intercept: don't fall through to mode switch
        }
    }

    // [Music mode] Special handling for volume adjust
    if (currentMode == MODE_MUSIC && isVolumeAdjusting) {
        isVolumeAdjusting = false; 
        myDFPlayer.volume(currentVolume);
        isUpdateRequired = true;
        return;
    }

    if (isTimeSetting) { isTimeSetting = false; isUpdateRequired = true; return; }

    // Mode cycling
    if (currentMode == MODE_EYES) currentMode = MODE_CLOCK;
    else if (currentMode == MODE_CLOCK) currentMode = MODE_MUSIC;
    else if (currentMode == MODE_MUSIC) currentMode = MODE_POMODORO;
    else if (currentMode == MODE_POMODORO) currentMode = MODE_ANSWER; 
    else if (currentMode == MODE_ANSWER) currentMode = MODE_EYES;     

    // Initialize mode state
    if (currentMode == MODE_POMODORO) pomoState = POMO_SETUP_WORK;
    if (currentMode == MODE_ANSWER) isAnswerRevealed = false;
    if (currentMode == MODE_MUSIC) {
        musicSelection = SEL_PLAY;
        isVolumeAdjusting = false;
    }

    isUpdateRequired = true; 
}

void handleBtnLLongPress() {
    lastInteractionTime = millis();

    // [Pomodoro] Long press: abort timer and return to setup
    if (currentMode == MODE_POMODORO) {
        Serial.println("BTN_L Long Press -> Pomodoro Reset to Setup");
        pomoState = POMO_SETUP_WORK;
        myDFPlayer.stop(); 
        isMusicPlaying = false; 
        isUpdateRequired = true;
        return; // Intercept: avoid executing global reset below
    }

    // --- Default long press behavior for other modes: return to Eyes ---
    // Sound retained as tactile feedback for system reset
    // myDFPlayer.play(TRACK_BUTTON); 
    Serial.println("BTN_L Long Press -> Reset to Eyes");
    currentMode = MODE_EYES;
    myDFPlayer.stop(); 
    isMusicPlaying = false;
    isAnswerRevealed = false; 
    isUpdateRequired = true;
}

void handleBtnRClick() {
    lastInteractionTime = millis();

    if (isTimeSetting) { isTimeSetting = false; isUpdateRequired = true; return; }

    if (currentMode == MODE_MUSIC) {
        if (isVolumeAdjusting) {
            myDFPlayer.volume(currentVolume); 
            isVolumeAdjusting = false;        
        } 
        else {
            switch (musicSelection) {
                case SEL_PLAY: 
                    if (isMusicPlaying) {
                        myDFPlayer.pause();
                        isMusicPlaying = false;
                    } else {
                        // Music starts from 03.mp3, so index offset is +2
                        // currentMusicTrack=1 plays track 3 (Mozart)
                        myDFPlayer.play(currentMusicTrack + 2); 
                        isMusicPlaying = true;
                    }
                    break;

                case SEL_NEXT: 
                    currentMusicTrack++;
                    if (currentMusicTrack > totalTracks) currentMusicTrack = 1; 
                    if (isMusicPlaying) {
                        myDFPlayer.play(currentMusicTrack + 2);
                    }
                    isUpdateRequired = true;
                    break;

                case SEL_PREV: 
                    currentMusicTrack--;
                    if (currentMusicTrack < 1) currentMusicTrack = totalTracks;
                    if (isMusicPlaying) {
                        myDFPlayer.play(currentMusicTrack + 2);
                    }
                    isUpdateRequired = true;
                    break;

                case SEL_VOL: 
                    isVolumeAdjusting = true; 
                    break;
            }
        }
        isUpdateRequired = true;
    } 
    else if (currentMode == MODE_POMODORO) {
        isUpdateRequired = true;
        switch (pomoState) {
            case POMO_SETUP_WORK:  pomoState = POMO_SETUP_SHORT; break;
            case POMO_SETUP_SHORT: pomoState = POMO_SETUP_LONG;  break;
            case POMO_SETUP_LONG:  
                pomoTimerSeconds = (long)pomoWorkTime * 60;
                pomoTotalDuration = pomoTimerSeconds;
                pomoRoundCounter = 0; 
                pomoState = POMO_RUNNING; 
                break;
            case POMO_RUNNING:       pomoState = POMO_PAUSED; break;
            case POMO_PAUSED:        pomoState = POMO_RUNNING; break;
            case POMO_BREAK_RUNNING: pomoState = POMO_BREAK_PAUSED; break;
            case POMO_BREAK_PAUSED:  pomoState = POMO_BREAK_RUNNING; break;
            default: break;
        }
    }
    else if (currentMode == MODE_ANSWER) {
        if (!isAnswerRevealed && !isAnswerSpinning) {
            // Initialize reel with random starting indices
            for (int i = 0; i < 5; i++) {
                answerReelIndices[i] = random(0, answers_count);
            }
            answerSpinOffset = 0.0;
            answerSpinStartTime = millis();
            isAnswerSpinning = true;
            myDFPlayer.play(TRACK_BUTTON); // Play button sound for feedback
            isUpdateRequired = true;
        }
    }
}

void handleBtnRLongPress() {
    lastInteractionTime = millis();

    if (currentMode == MODE_CLOCK) {
        isTimeSetting = !isTimeSetting;
        isUpdateRequired = true;
        // myDFPlayer.play(TRACK_BUTTON); 
    }
}

// --- Motion detection and eye control ---
void checkSensorsAndCommandEyes() {
    if (isActionInProgress) {
        if (millis() - actionStartTime > 1000) { isActionInProgress = false; }
        return; 
    }
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    float totalAccel = sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z);

    bool hasInteracted = false;
    
    // Static variables to track shake and interaction states
    static bool wasTilted = false;
    static unsigned long tiltEndTime = 0;
    static bool pendingHappyFeedback = false;



    // Check if in tilt/shake state
    bool isTilted = (a.acceleration.x > 3.0 || a.acceleration.x < -3.0);

    // 1. Violent shake (Confused)
    if (totalAccel > 30.0) { 
        hasInteracted = true; isActionInProgress = true; actionStartTime = millis();
        roboEyes.setIdleMode(false);
        roboEyes.anim_confused(); roboEyes.anim_confused(); roboEyes.anim_confused();
        
        // Reset all interaction timer states
        wasTilted = false; 
        pendingHappyFeedback = false;
        
    }
    // 2. Normal tilt/shake (look left/right)
    else if (isTilted) { 
        hasInteracted = true; 
        roboEyes.setIdleMode(false); 
        if (a.acceleration.x > 3.0) roboEyes.setPosition(W);
        else roboEyes.setPosition(E);
        
        wasTilted = true;
        pendingHappyFeedback = false; // If shaking continuously, keep resetting wait state
        
    }
    // 3. Return to stable state
    else { 
        if (!isBotTired) roboEyes.setIdleMode(true); 
        
        // Detect the exact moment shaking "just finished"
        if (wasTilted) {
            wasTilted = false;
            tiltEndTime = millis();
            pendingHappyFeedback = true; // Prepare to trigger delayed Happy feedback
        }
    }

    if (pendingHappyFeedback && (millis() - tiltEndTime >= 500)) {
        roboEyes.setHeight(35, 35); 
        roboEyes.setWidth(30, 30);
        roboEyes.setBorderradius(20, 20);
        
        // roboEyes.setPosition(DEFAULT); // Ensure eyes are centered
        roboEyes.setMood(HAPPY);
        pendingHappyFeedback = false;  // Trigger complete
    }



    if (hasInteracted) lastInteractionTime = millis();
}

// --- Pomodoro logic: added music resume feature ---
void updatePomodoroTimer() {
    if (currentMode != MODE_POMODORO) return;

    if (pomoState == POMO_WAIT_ALARM) {
        if (millis() - alarmWaitStartTime < 800) return;
        
        // Wait for BUSY signal to go HIGH (means audio playback finished)
        if (digitalRead(DFPLAYER_BUSY) == HIGH) { 
            pomoState = pendingNextState;       
            pomoTimerSeconds = pendingNextTimer;
            pomoTotalDuration = pendingNextTimer;
            isUpdateRequired = true;            
            lastPomoTick = millis(); 

            // If music was playing before alarm, resume playing now
            if (wasMusicPlayingBeforeAlarm) {
                if (currentMusicTrack == 1) myDFPlayer.play(TRACK_MOZART); 
                else myDFPlayer.play(currentMusicTrack + 3);
                isMusicPlaying = true;
                wasMusicPlayingBeforeAlarm = false; // Reset state
            }
        }
        return; 
    }

    if (pomoState == POMO_RUNNING || pomoState == POMO_BREAK_RUNNING) {
        if (millis() - lastPomoTick >= 1000) {
            lastPomoTick = millis();
            if (pomoTimerSeconds > 0) {
                pomoTimerSeconds--; isUpdateRequired = true; 
            } else {
                if (pomoState == POMO_RUNNING) {
                    pomoRoundCounter++; pendingNextState = POMO_BREAK_RUNNING;
                    pendingNextTimer = (pomoRoundCounter % 3 == 0) ? (long)pomoLongBreak * 60 : (long)pomoShortBreak * 60;
                } else { 
                    pendingNextState = POMO_RUNNING; pendingNextTimer = (long)pomoWorkTime * 60; 
                }
                
                // Record current music state
                wasMusicPlayingBeforeAlarm = isMusicPlaying; 
                
                myDFPlayer.play(TRACK_ALARM); 
                isMusicPlaying = false; // Temporarily mark as stopped to prevent UI showing play state
                
                pomoState = POMO_WAIT_ALARM; 
                alarmWaitStartTime = millis(); 
                isUpdateRequired = true; 
            }
        }
    }
}

// --- Main Task ---
void TaskSensorsCode(void * parameter) {
    pinMode(BTN_L_PIN, INPUT_PULLUP);
    pinMode(BTN_R_PIN, INPUT_PULLUP);
    pinMode(DFPLAYER_BUSY, INPUT); 

    btnL.attachClick(handleBtnLClick);
    btnL.attachLongPressStart(handleBtnLLongPress);
    btnR.attachClick(handleBtnRClick);
    btnR.attachLongPressStart(handleBtnRLongPress); 
    
    
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encoder.attachHalfQuad(ENC_DT, ENC_CLK);
    encoder.setCount(0);

    Wire.begin(IMU_SDA, IMU_SCL);      
    I2C_RTC.begin(RTC_SDA, RTC_SCL);   

    mpu.begin();
    rtc.begin(&I2C_RTC);

    // ==========================================
    // --- Auto time sync code ---
    // ==========================================
    DateTime now = rtc.now();
    // If year < 2026, it means RTC module never set or lost power
    if (now.year() < 2026) { 
        Serial.println("RTC time not set, writing compile time...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        currentDateTime = rtc.now(); // Ensure variable sync immediately
    }
    // ==========================================

    randomSeed(analogRead(34));

    mySerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
    myDFPlayer.begin(mySerial, false, false); 
    myDFPlayer.volume(currentVolume); 

    lastInteractionTime = millis();

    for(;;) {
      
        btnL.tick(); btnR.tick();

        // Periodically clear DFPlayer Serial buffer
        // Prevents third-party chips from crashing due to UART congestion from too many status messages
        while (mySerial.available() > 0) {
            mySerial.read(); 
        }

        // BUSY pin state monitoring and UI sync
        // Ensures actual playback state matches UI state in Music mode
        if (currentMode == MODE_MUSIC && isMusicPlaying) {
            // Give DFPlayer 1 sec command buffer time to prevent BUSY pin not pulling LOW in time after play
            if (millis() - lastInteractionTime > 1000) { 
                if (digitalRead(DFPLAYER_BUSY) == HIGH) {
                    // If program thinks it's playing, but BUSY is HIGH, it means:
                    // 1. Music finished naturally
                    // 2. DFPlayer crashed from undervoltage reset
                    
                    Serial.println("Track finished or DFPlayer reset!");
                    isMusicPlaying = false; // Sync state
                    isUpdateRequired = true; // Trigger UI update to switch pause back to play icon
                }
            }
        }

        // Set delayed Flash write logic (write 2 sec after stop to protect Flash lifespan)
        if (settingsNeedSave && (millis() - lastSettingChangeTime > 2000)) {
            prefs.begin("bot-settings", false);
            prefs.putInt("bright", displayBrightness);
            prefs.putInt("vol", currentVolume);
            prefs.putInt("pomoW", pomoWorkTime);
            prefs.putInt("pomoS", pomoShortBreak);
            prefs.putInt("pomoL", pomoLongBreak);
            prefs.end();
            settingsNeedSave = false;
            Serial.println("All Settings saved to Flash!");
        }

        long newPos = encoder.getCount() / 2;
        if (newPos != oldEncPos) {
            int dir = (newPos > oldEncPos) ? 1 : -1;
            oldEncPos = newPos;
            lastInteractionTime = millis();


            // Case A: Clock mode AND adjusting time -> only adjust time
            if (currentMode == MODE_CLOCK && isTimeSetting) {
                rtc.adjust(rtc.now() + TimeSpan(0, 0, dir, 0)); 
                currentDateTime = rtc.now(); 
                isUpdateRequired = true;
            }
            // Case B: (Eyes mode OR Clock mode) AND NOT adjusting time -> only adjust brightness
            else if ((currentMode == MODE_EYES || currentMode == MODE_CLOCK) && !isTimeSetting) {
                displayBrightness = constrain(displayBrightness + (dir * 15), 10, 255);
                isBrightnessChanged = true; 
                lastSettingChangeTime = millis(); settingsNeedSave = true; // Trigger save
            }
            
            // Music mode logic
            if (currentMode == MODE_MUSIC) {
                if (isVolumeAdjusting) {
                    currentVolume = constrain(currentVolume + dir, 0, 30);
                    lastSettingChangeTime = millis(); settingsNeedSave = true; // Trigger save
                } 
                else {
                    // Menu selection logic
                    int selection = (int)musicSelection + dir;
                    if (selection > 3) selection = 0; 
                    if (selection < 0) selection = 3; 
                    musicSelection = (MusicSelection)selection;
                }
                isUpdateRequired = true;
            }
            
            // Pomodoro setup logic
            else if (currentMode == MODE_POMODORO) {
                int* targetTime = nullptr;
                if (pomoState == POMO_SETUP_WORK) targetTime = &pomoWorkTime;
                else if (pomoState == POMO_SETUP_SHORT) targetTime = &pomoShortBreak;
                else if (pomoState == POMO_SETUP_LONG) targetTime = &pomoLongBreak;

                if (targetTime != nullptr) {
                    if (dir > 0) { if (*targetTime < 5) *targetTime = 5; else *targetTime += 5; }
                    else { if (*targetTime <= 5) *targetTime = 1; else *targetTime -= 5; }
                    *targetTime = constrain(*targetTime, 1, 60);
                    lastSettingChangeTime = millis(); settingsNeedSave = true; // Trigger save
                }
                isUpdateRequired = true;
            }
        }

        // RTC and Pomodoro update
        if (millis() - lastRtcUpdate > 1000) {
            if (currentMode == MODE_CLOCK) { currentDateTime = rtc.now(); isUpdateRequired = true; }
            lastRtcUpdate = millis();
        }

        updatePomodoroTimer(); 

        // Eyes mode and fatigue check
        if (currentMode == MODE_EYES) {
             checkSensorsAndCommandEyes(); 
             
             // Enter tired mode after 30 min of no operation (30*60*1000 ms)
             if (millis() - lastInteractionTime > 1800000) { 
                if (!isBotTired) { 
                    isBotTired = true; 
                    // Specific expression changes handled by Task_UI.cpp state machine
                }
            } else {
                if (isBotTired) { 
                    isBotTired = false; 
                    isUpdateRequired = true;
                }
            }
        }

        unsigned long idleTime = millis() - lastInteractionTime;

        // 1. Auto-return logic (idle for 2 mins = 120,000 ms)
        if (idleTime > 120000) {
            bool shouldReturn = false;
            // Check if in a mode that needs returning
            if (currentMode == MODE_MUSIC || currentMode == MODE_ANSWER) {
                shouldReturn = true;
            } else if (currentMode == MODE_POMODORO && 
                      (pomoState == POMO_SETUP_WORK || pomoState == POMO_SETUP_SHORT || pomoState == POMO_SETUP_LONG)) {
                shouldReturn = true;
            }
            
            if (shouldReturn) {
                currentMode = MODE_EYES;
                isUpdateRequired = true;
                // Reset interaction time to avoid early trigger of 1.5 hour sleep
                lastInteractionTime = millis(); 
                Serial.println("Auto-Return to MODE_EYES");
            }
        }

        // 2. Sleep logic
        unsigned long sleepThreshold = 5400000; // Default 1.5 hours (5,400,000 ms)
        
        // If Pomodoro is running (including pause, break, wait alarm), extend sleep threshold to 4 hours (14,400,000 ms)
        if (currentMode == MODE_POMODORO && 
           (pomoState == POMO_RUNNING || pomoState == POMO_PAUSED || 
            pomoState == POMO_BREAK_RUNNING || pomoState == POMO_BREAK_PAUSED || pomoState == POMO_WAIT_ALARM)) {
            sleepThreshold = 14400000; 
        }

        if (idleTime > sleepThreshold) {
            isAsleep = true;
            isUpdateRequired = true; // Notify UI Task to prepare turning off display
            Serial.println("Minibot goes to Light Sleep...");
            
            // Give UI Task enough time (200ms) to clear and turn off OLED
            vTaskDelay(pdMS_TO_TICKS(200)); 
            
            // --- Setup wakeup sources (dynamically detect current voltage and wakeup on opposite state) ---
            
            // Handle Left Button
            if (digitalRead(BTN_L_PIN) == HIGH) {
                gpio_wakeup_enable((gpio_num_t)BTN_L_PIN, GPIO_INTR_LOW_LEVEL);
            } else {
                gpio_wakeup_enable((gpio_num_t)BTN_L_PIN, GPIO_INTR_HIGH_LEVEL);
            }

            // Handle Right Button
            if (digitalRead(BTN_R_PIN) == HIGH) {
                gpio_wakeup_enable((gpio_num_t)BTN_R_PIN, GPIO_INTR_LOW_LEVEL);
            } else {
                gpio_wakeup_enable((gpio_num_t)BTN_R_PIN, GPIO_INTR_HIGH_LEVEL);
            }

            // Handle Encoder DT (fixes instant wake-up issue when resting at LOW)
            if (digitalRead(ENC_DT) == HIGH) {
                gpio_wakeup_enable((gpio_num_t)ENC_DT, GPIO_INTR_LOW_LEVEL);
            } else {
                gpio_wakeup_enable((gpio_num_t)ENC_DT, GPIO_INTR_HIGH_LEVEL);
            }

            // Handle Encoder CLK (fixes instant wake-up issue when resting at LOW)
            if (digitalRead(ENC_CLK) == HIGH) {
                gpio_wakeup_enable((gpio_num_t)ENC_CLK, GPIO_INTR_LOW_LEVEL);
            } else {
                gpio_wakeup_enable((gpio_num_t)ENC_CLK, GPIO_INTR_HIGH_LEVEL);
            }

            esp_sleep_enable_gpio_wakeup();
            
            // Enter Light Sleep! (ESP32 CPU freezes here, execution is paused)
            esp_light_sleep_start(); 
            
            // ==========================================
            // --- After waking up, code execution seamlessly resumes from this line ---
            // ==========================================
            Serial.println("Minibot Woke Up!");
            
            // Clear any residual button or encoder interrupt states to prevent accidental triggers on wake up
            while (mySerial.available() > 0) mySerial.read(); 
            
            lastInteractionTime = millis(); // Refresh timer
            isAsleep = false;
            isUpdateRequired = true; // Notify UI Task to resume display
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}