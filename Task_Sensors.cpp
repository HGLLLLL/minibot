#include "Globals.h"
#include "HardwareSerial.h"
#include <Preferences.h>

// 建立 Preferences 物件用於掉電記憶
Preferences prefs;

// 宣告 Task 函數
void TaskSensorsCode(void * parameter);

HardwareSerial mySerial(2); 

// --- 內部控制變數 ---
long oldEncPos = 0;
unsigned long lastRtcUpdate = 0;
unsigned long lastInteractionTime = 0; 
unsigned long actionStartTime = 0;
unsigned long lastPomoTick = 0; 

// --- 系統設定記憶相關變數 ---
unsigned long lastSettingChangeTime = 0; 
bool settingsNeedSave = false;

// bool isTimeSetting = false; // 調時模式旗標

// 專為番茄鐘「播完音效再切換」設計的暫存變數
PomoState pendingNextState; 
long pendingNextTimer = 0;
unsigned long alarmWaitStartTime = 0;

// 函數宣告
void checkSensorsAndCommandEyes();

// --- 按鈕回調函數 ---

void handleBtnLClick() {
    lastInteractionTime = millis();

    // [番茄鐘] 設定階段：返回上一步
    if (currentMode == MODE_POMODORO) {
        if (pomoState == POMO_SETUP_SHORT) { pomoState = POMO_SETUP_WORK; isUpdateRequired = true; return; }
        if (pomoState == POMO_SETUP_LONG)  { pomoState = POMO_SETUP_SHORT; isUpdateRequired = true; return; }
        if (pomoState != POMO_SETUP_WORK) { return; } 
    }

    // [解答之書] 返回
    if (currentMode == MODE_ANSWER) {
        if (isAnswerRevealed) {          // 確保只有在「顯示答案」時按左鍵才會作用
            isAnswerRevealed = false; 
            isUpdateRequired = true;
            return;                      // 攔截，不讓程式跑到下方的模式切換
        }
    }

    // [音樂模式] 特殊處理
    if (currentMode == MODE_MUSIC && isVolumeAdjusting) {
        isVolumeAdjusting = false; 
        myDFPlayer.volume(currentVolume);
        isUpdateRequired = true;
        return;
    }

    if (isTimeSetting) { isTimeSetting = false; isUpdateRequired = true; return; }

    // 模式循環切換
    if (currentMode == MODE_EYES) currentMode = MODE_CLOCK;
    else if (currentMode == MODE_CLOCK) currentMode = MODE_MUSIC;
    else if (currentMode == MODE_MUSIC) currentMode = MODE_POMODORO;
    else if (currentMode == MODE_POMODORO) currentMode = MODE_ANSWER; 
    else if (currentMode == MODE_ANSWER) currentMode = MODE_EYES;     

    // 初始化模式狀態
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

    // [番茄鐘] 專屬長按行為：中斷計時，返回設定頁面
    if (currentMode == MODE_POMODORO) {
        Serial.println("BTN_L Long Press -> Pomodoro Reset to Setup");
        pomoState = POMO_SETUP_WORK;
        myDFPlayer.stop(); 
        isMusicPlaying = false; 
        isUpdateRequired = true;
        return; // 攔截，避免執行下方的全域重置
    }

    // --- 其他模式的預設長按行為：回到眼睛模式 ---
    // 長按保留音效作為系統重置的觸覺反饋
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
                        // 因為你的音樂是從 03.mp3 開始，所以索引要 + 2
                        // currentMusicTrack 為 1 時，撥放第 3 首 (Mozart)
                        myDFPlayer.play(currentMusicTrack + 2); 
                        isMusicPlaying = true;
                    }
                    break;

                case SEL_NEXT: 
                    currentMusicTrack++;
                    if (currentMusicTrack > totalTracks) currentMusicTrack = 1; 
                    isUpdateRequired = true;
                    break;

                case SEL_PREV: 
                    currentMusicTrack--;
                    if (currentMusicTrack < 1) currentMusicTrack = totalTracks;
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
        if (!isAnswerRevealed) {
            int randomIndex = random(0, answers_count);
            // 修正 PROGMEM 讀取方式
            currentAnswer = (const char*)pgm_read_ptr(&answers_pool[randomIndex]); 
            isAnswerRevealed = true;
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

// --- 偵測與眼睛控制 ---
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
    else if (a.acceleration.x > 3.0) { hasInteracted = true; roboEyes.setIdleMode(false); roboEyes.setPosition(W); }
    else if (a.acceleration.x < -3.0) { hasInteracted = true; roboEyes.setIdleMode(false); roboEyes.setPosition(E); }
    else { 
        if (!isBotTired) roboEyes.setIdleMode(true); 
    }

    if (hasInteracted) lastInteractionTime = millis();
}

// --- 番茄鐘邏輯：增加音樂恢復功能 ---
void updatePomodoroTimer() {
    if (currentMode != MODE_POMODORO) return;

    if (pomoState == POMO_WAIT_ALARM) {
        if (millis() - alarmWaitStartTime < 800) return;
        
        // 等待 BUSY 訊號變高 (代表音效播放完畢)
        if (digitalRead(DFPLAYER_BUSY) == HIGH) { 
            pomoState = pendingNextState;       
            pomoTimerSeconds = pendingNextTimer;
            pomoTotalDuration = pendingNextTimer;
            isUpdateRequired = true;            
            lastPomoTick = millis(); 

            // 如果響鈴前有在聽歌，現在恢復播放
            if (wasMusicPlayingBeforeAlarm) {
                if (currentMusicTrack == 1) myDFPlayer.play(TRACK_MOZART); 
                else myDFPlayer.play(currentMusicTrack + 3);
                isMusicPlaying = true;
                wasMusicPlayingBeforeAlarm = false; // 重置狀態
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
                
                // 記錄當前音樂狀態
                wasMusicPlayingBeforeAlarm = isMusicPlaying; 
                
                myDFPlayer.play(TRACK_ALARM); 
                isMusicPlaying = false; // 暫時標記為停止，避免 UI 顯示播放中
                
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
    // --- 加入自動校時程式碼 ---
    // ==========================================
    DateTime now = rtc.now();
    // 如果年份小於 2026，代表 RTC 模組從未設定過時間或已經掉電重置
    if (now.year() < 2026) { 
        Serial.println("RTC 時間未設定，寫入編譯當下時間...");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        currentDateTime = rtc.now(); // 確保變數立即同步
    }
    // ==========================================

    randomSeed(analogRead(34));

    mySerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
    myDFPlayer.begin(mySerial, false, false); 
    myDFPlayer.volume(currentVolume); 

    lastInteractionTime = millis();

    for(;;) {
      
        btnL.tick(); btnR.tick();

        // 定期清空 DFPlayer 的 Serial 緩衝區
        // 防止副廠晶片因為發送太多狀態訊息導致 UART 塞車而罷工
        while (mySerial.available() > 0) {
            mySerial.read(); 
        }

        // BUSY 引腳狀態監控與 UI 同步
        // 確保音樂模式下，實際播放狀態與 UI 狀態一致
        if (currentMode == MODE_MUSIC && isMusicPlaying) {
            // 給予 DFPlayer 1 秒的指令緩衝時間，避免剛按下播放時 BUSY 腳還來不及拉低(LOW)
            if (millis() - lastInteractionTime > 1000) { 
                if (digitalRead(DFPLAYER_BUSY) == HIGH) {
                    // 如果程式認為正在播，但 BUSY 卻是 HIGH，代表:
                    // 1. 音樂自然播完了 
                    // 2. DFPlayer 欠壓重置當機了
                    
                    Serial.println("Track finished or DFPlayer reset!");
                    isMusicPlaying = false; // 同步狀態
                    isUpdateRequired = true; // 觸發 UI 更新把暫停鍵變回播放鍵
                }
            }
        }

        // 設定延遲寫入 Flash 邏輯 (停止操作 2 秒後才寫入，保護 Flash 壽命)
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


            // 情況 A: 時鐘模式 且 正在調時間 -> 只調時間
            if (currentMode == MODE_CLOCK && isTimeSetting) {
                rtc.adjust(rtc.now() + TimeSpan(0, 0, dir, 0)); 
                currentDateTime = rtc.now(); 
                isUpdateRequired = true;
            }
            // 情況 B: (陪伴模式 或 時鐘模式) 且 「不在」調時間 -> 只調亮度
            else if ((currentMode == MODE_EYES || currentMode == MODE_CLOCK) && !isTimeSetting) {
                displayBrightness = constrain(displayBrightness + (dir * 15), 10, 255);
                isBrightnessChanged = true; 
                lastSettingChangeTime = millis(); settingsNeedSave = true; // 觸發儲存
            }
            
            // 音樂模式邏輯
            if (currentMode == MODE_MUSIC) {
                if (isVolumeAdjusting) {
                    currentVolume = constrain(currentVolume + dir, 0, 30);
                    lastSettingChangeTime = millis(); settingsNeedSave = true; // 觸發儲存
                } 
                else {
                    // 切換選單邏輯
                    int selection = (int)musicSelection + dir;
                    if (selection > 3) selection = 0; 
                    if (selection < 0) selection = 3; 
                    musicSelection = (MusicSelection)selection;
                }
                isUpdateRequired = true;
            }
            
            // 番茄鐘設定邏輯
            else if (currentMode == MODE_POMODORO) {
                int* targetTime = nullptr;
                if (pomoState == POMO_SETUP_WORK) targetTime = &pomoWorkTime;
                else if (pomoState == POMO_SETUP_SHORT) targetTime = &pomoShortBreak;
                else if (pomoState == POMO_SETUP_LONG) targetTime = &pomoLongBreak;

                if (targetTime != nullptr) {
                    if (dir > 0) { if (*targetTime < 5) *targetTime = 5; else *targetTime += 5; }
                    else { if (*targetTime <= 5) *targetTime = 1; else *targetTime -= 5; }
                    *targetTime = constrain(*targetTime, 1, 60);
                    lastSettingChangeTime = millis(); settingsNeedSave = true; // 觸發儲存
                }
                isUpdateRequired = true;
            }
        }

        // RTC 與番茄鐘更新
        if (millis() - lastRtcUpdate > 1000) {
            if (currentMode == MODE_CLOCK) { currentDateTime = rtc.now(); isUpdateRequired = true; }
            lastRtcUpdate = millis();
        }

        updatePomodoroTimer(); 

        // 陪伴模式與疲勞判定
        if (currentMode == MODE_EYES) {
             checkSensorsAndCommandEyes(); 
             
             // 30 min 無操作進入疲勞模式 (30*60*1000 ms)
             if (millis() - lastInteractionTime > 1800000) { 
                if (!isBotTired) { 
                    isBotTired = true; 
                    // 具體表情變換由 Task_UI.cpp 的狀態機處理
                }
            } else {
                if (isBotTired) { 
                    isBotTired = false; 
                    isUpdateRequired = true;
                }
            }
        }

        unsigned long idleTime = millis() - lastInteractionTime;

        // 1. 自動返回邏輯 (閒置超過 2分鐘 = 120,000 ms)
        if (idleTime > 120000) {
            bool shouldReturn = false;
            // 判斷是否在需要返回的模式
            if (currentMode == MODE_MUSIC || currentMode == MODE_ANSWER) {
                shouldReturn = true;
            } else if (currentMode == MODE_POMODORO && 
                      (pomoState == POMO_SETUP_WORK || pomoState == POMO_SETUP_SHORT || pomoState == POMO_SETUP_LONG)) {
                shouldReturn = true;
            }
            
            if (shouldReturn) {
                currentMode = MODE_EYES;
                isUpdateRequired = true;
                // 重置互動時間，避免 1.5 小時的睡眠時間被提早觸發
                lastInteractionTime = millis(); 
                Serial.println("Auto-Return to MODE_EYES");
            }
        }

        // 2. 睡眠邏輯
        unsigned long sleepThreshold = 5400000; // 預設 1.5 小時 (5,400,000 ms)
        
        // 若番茄鐘正在運行 (含暫停、休息、等響鈴)，自動睡眠延長至 4 小時 (14,400,000 ms)
        if (currentMode == MODE_POMODORO && 
           (pomoState == POMO_RUNNING || pomoState == POMO_PAUSED || 
            pomoState == POMO_BREAK_RUNNING || pomoState == POMO_BREAK_PAUSED || pomoState == POMO_WAIT_ALARM)) {
            sleepThreshold = 14400000; 
        }

        if (idleTime > sleepThreshold) {
            isAsleep = true;
            isUpdateRequired = true; // 通知 UI Task 準備關閉螢幕
            Serial.println("Minibot goes to Light Sleep...");
            
            // 給 UI Task 充分的時間 (200ms) 把 OLED 畫面清空並關閉
            vTaskDelay(pdMS_TO_TICKS(200)); 
            
            // --- 設定喚醒源 (動態偵測當下電位，設定為相反狀態時喚醒) ---
            
            // 處理左按鈕
            if (digitalRead(BTN_L_PIN) == HIGH) {
                gpio_wakeup_enable((gpio_num_t)BTN_L_PIN, GPIO_INTR_LOW_LEVEL);
            } else {
                gpio_wakeup_enable((gpio_num_t)BTN_L_PIN, GPIO_INTR_HIGH_LEVEL);
            }

            // 處理右按鈕
            if (digitalRead(BTN_R_PIN) == HIGH) {
                gpio_wakeup_enable((gpio_num_t)BTN_R_PIN, GPIO_INTR_LOW_LEVEL);
            } else {
                gpio_wakeup_enable((gpio_num_t)BTN_R_PIN, GPIO_INTR_HIGH_LEVEL);
            }

            // 處理旋鈕 DT (解決停在 LOW 導致秒醒的問題)
            if (digitalRead(ENC_DT) == HIGH) {
                gpio_wakeup_enable((gpio_num_t)ENC_DT, GPIO_INTR_LOW_LEVEL);
            } else {
                gpio_wakeup_enable((gpio_num_t)ENC_DT, GPIO_INTR_HIGH_LEVEL);
            }

            // 處理旋鈕 CLK (解決停在 LOW 導致秒醒的問題)
            if (digitalRead(ENC_CLK) == HIGH) {
                gpio_wakeup_enable((gpio_num_t)ENC_CLK, GPIO_INTR_LOW_LEVEL);
            } else {
                gpio_wakeup_enable((gpio_num_t)ENC_CLK, GPIO_INTR_HIGH_LEVEL);
            }

            esp_sleep_enable_gpio_wakeup();
            
            // 進入 Light Sleep！(ESP32 的 CPU 會在這裡凍結，暫停執行)
            esp_light_sleep_start(); 
            
            // ==========================================
            // --- 被喚醒後，程式會從這行「無縫接軌」繼續執行 ---
            // ==========================================
            Serial.println("Minibot Woke Up!");
            
            // 清除可能殘留的按鈕或旋鈕中斷狀態，避免剛醒來就誤觸發動作
            while (mySerial.available() > 0) mySerial.read(); 
            
            lastInteractionTime = millis(); // 刷新計時器
            isAsleep = false;
            isUpdateRequired = true; // 通知 UI Task 恢復螢幕
        }

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}