#include "Globals.h"
#include "HardwareSerial.h"

// 宣告 Task 函數
void TaskSensorsCode(void * parameter);

HardwareSerial mySerial(2); 

// --- 變數 ---
long oldEncPos = 0;
unsigned long lastRtcUpdate = 0;
unsigned long lastInteractionTime = 0; 
unsigned long actionStartTime = 0;
unsigned long lastPomoTick = 0; 

// 函數宣告
void checkSensorsAndCommandEyes();

// --- 按鈕回調函數 ---

void handleBtnLClick() {
    // [番茄鐘] 設定階段：返回上一步
    if (currentMode == MODE_POMODORO) {
        if (pomoState == POMO_SETUP_SHORT) { pomoState = POMO_SETUP_WORK; isUpdateRequired = true; return; }
        if (pomoState == POMO_SETUP_LONG)  { pomoState = POMO_SETUP_SHORT; isUpdateRequired = true; return; }
        if (pomoState != POMO_SETUP_WORK) { return; } 
    }

    // [解答之書] 返回
    if (currentMode == MODE_ANSWER) {
        isAnswerRevealed = false; 
    }

    // [音樂模式] 特殊處理：如果在調音量，按左鍵視為「確認/退出」
    if (currentMode == MODE_MUSIC && isVolumeAdjusting) {
        isVolumeAdjusting = false; 
        myDFPlayer.volume(currentVolume); // 退出時也要確保音量設定生效
        isUpdateRequired = true;
        return;
    }

    // 模式循環切換
    // 順序：Eyes -> Clock -> Music -> Pomodoro -> Answer -> Eyes
    if (currentMode == MODE_EYES) currentMode = MODE_CLOCK;
    else if (currentMode == MODE_CLOCK) currentMode = MODE_MUSIC;
    else if (currentMode == MODE_MUSIC) currentMode = MODE_POMODORO;
    else if (currentMode == MODE_POMODORO) currentMode = MODE_ANSWER; 
    else if (currentMode == MODE_ANSWER) currentMode = MODE_EYES;     

    // 重置各模式狀態
    if (currentMode == MODE_POMODORO) pomoState = POMO_SETUP_WORK;
    if (currentMode == MODE_ANSWER) isAnswerRevealed = false;
    
    // [新增] 進入音樂模式時，重置為選中 Play
    if (currentMode == MODE_MUSIC) {
        musicSelection = SEL_PLAY;
        isVolumeAdjusting = false;
    }

    isUpdateRequired = true; 
}

void handleBtnLLongPress() {
    Serial.println("BTN_L Long Press -> Reset to Eyes");
    currentMode = MODE_EYES;
    myDFPlayer.stop(); // 停止鈴聲/音樂
    isMusicPlaying = false;
    isAnswerRevealed = false; 
    isUpdateRequired = true;
}

void handleBtnRClick() {
    Serial.println("BTN_R Clicked");
    lastInteractionTime = millis(); 

    if (currentMode == MODE_MUSIC) {
        // --- 音樂模式邏輯 ---
        if (isVolumeAdjusting) {
            // [狀態：調整音量中] -> 按下右鍵：確認並執行音量調整，回到選單
            myDFPlayer.volume(currentVolume); // 硬體執行音量改變
            isVolumeAdjusting = false;        // 退出調整模式
        } 
        else {
            // [狀態：正常選單] -> 根據選中的按鈕執行動作
            switch (musicSelection) {
                case SEL_PLAY: // 播放/暫停
                    if (isMusicPlaying) {
                        myDFPlayer.pause();
                        isMusicPlaying = false;
                    } else {
                        // [修正] 改用 play(track) 而非 start()
                        // 確保播放的是螢幕上顯示的那一首，而不是記憶中暫停的那一首
                        myDFPlayer.play(currentMusicTrack);
                        isMusicPlaying = true;
                    }
                    break;

                case SEL_NEXT: // 下一首
                    currentMusicTrack++;
                    // 假設 SD 卡有 50 首
                    if (currentMusicTrack > 50) currentMusicTrack = 1; 
                    
                    // [修正] 取消自動播放
                    // 這裡只更新變數，不呼叫 myDFPlayer.play()
                    // 也不改變 isMusicPlaying 狀態 (保持原狀)
                    break;

                case SEL_PREV: // 上一首
                    currentMusicTrack--;
                    if (currentMusicTrack < 1) currentMusicTrack = 1;
                    
                    // [修正] 取消自動播放
                    break;

                case SEL_VOL: // 音量鍵
                    isVolumeAdjusting = true; // 進入調整模式
                    break;
            }
        }
        isUpdateRequired = true;
    } 
    else if (currentMode == MODE_EYES) {
        roboEyes.setMood(HAPPY);
    }
    else if (currentMode == MODE_POMODORO) {
        isUpdateRequired = true;
        switch (pomoState) {
            case POMO_SETUP_WORK:  pomoState = POMO_SETUP_SHORT; break;
            case POMO_SETUP_SHORT: pomoState = POMO_SETUP_LONG;  break;
            case POMO_SETUP_LONG:  
                pomoTimerSeconds = (long)pomoWorkTime * 60;
                pomoTotalDuration = pomoTimerSeconds;
                if(pomoTotalDuration <= 0) pomoTotalDuration = 60; 
                pomoRoundCounter = 0; 
                pomoState = POMO_RUNNING; 
                break;
            case POMO_RUNNING:       pomoState = POMO_PAUSED; break;
            case POMO_PAUSED:        pomoState = POMO_RUNNING; break;
            case POMO_BREAK_RUNNING: pomoState = POMO_BREAK_PAUSED; break;
            case POMO_BREAK_PAUSED:  pomoState = POMO_BREAK_RUNNING; break;
        }
    }
    else if (currentMode == MODE_ANSWER) {
        if (!isAnswerRevealed) {
            int randomIndex = random(0, answers_count);
            currentAnswer = answers_pool[randomIndex];
            isAnswerRevealed = true;
            isUpdateRequired = true;
        }
    }
}

// --- 眼睛互動邏輯 ---
void checkSensorsAndCommandEyes() {
    if (isActionInProgress) {
        if (millis() - actionStartTime > 1000) { isActionInProgress = false; }
        return; 
    }
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    float totalAccel = sqrt(a.acceleration.x * a.acceleration.x + a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z);

    bool hasInteracted = false;
    if (totalAccel > 30.0) { 
        hasInteracted = true; isActionInProgress = true; actionStartTime = millis();
        roboEyes.setIdleMode(false);
        roboEyes.anim_confused(); roboEyes.anim_confused(); roboEyes.anim_confused();
    }
    else if (a.acceleration.x > 5.0) { hasInteracted = true; roboEyes.setIdleMode(false); roboEyes.setPosition(E); }
    else if (a.acceleration.x < -5.0) { hasInteracted = true; roboEyes.setIdleMode(false); roboEyes.setPosition(W); }
    else { if (!isBotTired) roboEyes.setIdleMode(true); }

    if (hasInteracted) lastInteractionTime = millis();
}

// --- 番茄鐘倒數邏輯 (Watchdog Safe 版) ---
void updatePomodoroTimer() {
    if (currentMode != MODE_POMODORO) return;
    if (pomoState == POMO_RUNNING || pomoState == POMO_BREAK_RUNNING) {
        if (millis() - lastPomoTick >= 1000) {
            lastPomoTick = millis();
            if (pomoTimerSeconds > 0) {
                pomoTimerSeconds--;
                isUpdateRequired = true; 
            } else {
                long nextTimer = 0;
                int nextTotal = 0;
                PomoState nextState;

                if (pomoState == POMO_RUNNING) {
                    pomoRoundCounter++; 
                    if (pomoRoundCounter % 3 == 0) {
                        nextState = POMO_BREAK_RUNNING;
                        nextTimer = (long)pomoLongBreak * 60;
                    } else {
                        nextState = POMO_BREAK_RUNNING;
                        nextTimer = (long)pomoShortBreak * 60;
                    }
                } else { 
                    nextState = POMO_RUNNING;
                    nextTimer = (long)pomoWorkTime * 60;
                }
                if (nextTimer <= 0) nextTimer = 60; 
                nextTotal = nextTimer;
                pomoTimerSeconds = nextTimer;
                pomoTotalDuration = nextTotal;
                pomoState = nextState;
                isUpdateRequired = true; 
                myDFPlayer.play(TRACK_ALARM); 
                lastPomoTick = millis();
            }
        }
    }
}

// --- Main Task ---
void TaskSensorsCode(void * parameter) {
    pinMode(BTN_L_PIN, INPUT_PULLUP);
    pinMode(BTN_R_PIN, INPUT_PULLUP);
    btnL.attachClick(handleBtnLClick);
    btnL.attachLongPressStart(handleBtnLLongPress);
    btnR.attachClick(handleBtnRClick);
    
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encoder.attachHalfQuad(ENC_DT, ENC_CLK);
    encoder.setCount(0);

    Wire.begin(IMU_SDA, IMU_SCL);       
    I2C_RTC.begin(RTC_SDA, RTC_SCL);    

    if (!mpu.begin()) Serial.println("MPU Fail");
    else { mpu.setAccelerometerRange(MPU6050_RANGE_8_G); mpu.setGyroRange(MPU6050_RANGE_500_DEG); mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); }

    if (!rtc.begin(&I2C_RTC)) Serial.println("RTC Fail");

    randomSeed(analogRead(34)); 

    mySerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
    myDFPlayer.begin(mySerial, false, false); // Disable ACK
    myDFPlayer.volume(currentVolume); 

    lastInteractionTime = millis();

    for(;;) {
        btnL.tick(); btnR.tick();

        long newPos = encoder.getCount() / 2;
        if (newPos != oldEncPos) {
            int dir = (newPos > oldEncPos) ? 1 : -1;
            oldEncPos = newPos;
            lastInteractionTime = millis();
            
            if (currentMode == MODE_MUSIC) {
                // --- 音樂模式旋鈕邏輯 ---
                if (isVolumeAdjusting) {
                    // [調整音量模式]：只改變數，不改硬體
                    if (dir > 0) {
                        currentVolume++; if (currentVolume > 30) currentVolume = 30;
                    } else {
                        currentVolume--; if (currentVolume < 0) currentVolume = 0;
                    }
                } 
                else {
                    // [選單導航模式]：循環切換按鈕
                    // 順序：PLAY(0) -> NEXT(1) -> VOL(2) -> PREV(3)
                    int selection = (int)musicSelection;
                    selection += dir;
                    if (selection > 3) selection = 0; 
                    if (selection < 0) selection = 3; 
                    musicSelection = (MusicSelection)selection;
                }
                isUpdateRequired = true;
            }
            else if (currentMode == MODE_POMODORO) {
                // 番茄鐘時間設定
                int* targetTime = nullptr;
                int maxTime = 60;
                if (pomoState == POMO_SETUP_WORK) targetTime = &pomoWorkTime;
                else if (pomoState == POMO_SETUP_SHORT) targetTime = &pomoShortBreak;
                else if (pomoState == POMO_SETUP_LONG) targetTime = &pomoLongBreak;

                if (targetTime != nullptr) {
                    if (dir > 0) { 
                        if (*targetTime < 5) *targetTime = 5; 
                        else *targetTime += 5;                
                    } else { 
                        if (*targetTime <= 5) *targetTime = 1; 
                        else *targetTime -= 5;                 
                    }
                    if (*targetTime > maxTime) *targetTime = maxTime;
                    if (*targetTime < 1) *targetTime = 1;
                }
                isUpdateRequired = true;
            }
        }

        if (millis() - lastRtcUpdate > 1000) {
            if (currentMode == MODE_CLOCK) { currentDateTime = rtc.now(); isUpdateRequired = true; }
            lastRtcUpdate = millis();
        }

        updatePomodoroTimer(); 

        if (currentMode == MODE_EYES) {
             checkSensorsAndCommandEyes(); 
             if (millis() - lastInteractionTime > 300000) { 
                if (!isBotTired) { isBotTired = true; roboEyes.setHeight(10, 10); roboEyes.setCuriosity(false); roboEyes.setMood(DEFAULT); }
            } else {
                if (isBotTired) { isBotTired = false; roboEyes.setHeight(35, 35); roboEyes.setCuriosity(true); roboEyes.setIdleMode(true); roboEyes.setMood(DEFAULT); }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}