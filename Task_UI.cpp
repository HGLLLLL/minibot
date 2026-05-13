#include "Globals.h"
#include <U8g2_for_Adafruit_GFX.h>

#include "Ubuntu_10p_b.h"
#include "Ubuntu_14p_b.h"
#include "Ubuntu_18p_b.h"
#include "Ubuntu_24p_b.h"

void TaskUICode(void * parameter);
U8G2_FOR_ADAFRUIT_GFX u8g2_gfx;

// Helper for Adafruit GFX text width calculation
int getAdafruitTextWidth(const char* text) {
    int16_t x1, y1; uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return w;
}

int getAdafruitTextWidth(String text) {
    return getAdafruitTextWidth(text.c_str());
}

extern volatile bool isBrightnessChanged;
extern bool isTimeSetting;
// --- Bitmap Resources (XBM) ---

// 1. Arrow icons (retained)
static const unsigned char image_arrow_curved_left_down_bits[] = {0x18,0x7d,0x8f,0x07,0x0f};
static const unsigned char image_arrow_curved_right_down_bits[] = {0x18,0xbe,0xf1,0xe0,0xf0};

// 2. Music player base icons (outline)
static const unsigned char image_music_prev_bits[] = {
  0x00,0x00,0x0f,0xe0,0x09,0x90,0x09,0x8c,0x09,0x82,0x89,0x81,0x49,0x80,0x39,0x80,
  0x39,0x80,0x49,0x80,0x89,0x81,0x09,0x82,0x09,0x8c,0x09,0x90,0x0f,0xe0,0x00,0x00
};
static const unsigned char image_music_play_bits[] = {
  0x03,0x00,0x07,0x00,0x19,0x00,0x61,0x00,0x81,0x01,0x01,0x06,0x01,0x18,0x01,0x60,
  0x01,0x18,0x01,0x06,0x81,0x01,0x61,0x00,0x19,0x00,0x07,0x00,0x03,0x00,0x00,0x00
};
// Pause button base icon (12x16)
static const unsigned char image_music_pause_bits[] = {
  0x9f,0x0f,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,
  0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x9f,0x0f,0x00,0x00
};
static const unsigned char image_music_next_bits[] = {
  0x00,0x00,0x07,0xf0,0x09,0x90,0x31,0x90,0x41,0x90,0x81,0x91,0x01,0x92,0x01,0x9c,
  0x01,0x9c,0x01,0x92,0x81,0x91,0x41,0x90,0x31,0x90,0x09,0x90,0x07,0xf0,0x00,0x00
};
static const unsigned char image_Volup_bits[] = {0x48,0x8c,0xaf,0xaf,0x8c,0x48};

// 3. Music player fill layers (selected state)
static const unsigned char image_fill_prev[] = { 
  0x00,0x20,0x03,0x30,0x03,0x38,0x03,0x3e,0x03,0x3f,0xc3,0x3f,0xe3,0x3f,0xe3,0x7f,
  0xc3,0x7f,0x03,0x7f,0x03,0x7e,0x03,0x7a,0x03,0x70,0x00,0x40
};
static const unsigned char image_fill_play[] = { 
  0x04,0x00,0x07,0x00,0x1e,0x00,0x7e,0x00,0xfe,0x05,0xfe,0x0f,0xfe,0x1f,0xfe,0x07,
  0xfe,0x07,0xfe,0x00,0x5e,0x00,0x1e,0x00,0x04,0x00
};
// Pause button fill layer (11x15)
static const unsigned char image_fill_pause[] = {
  0x02,0x01,0x87,0x03,0x87,0x03,0x87,0x03,0x87,0x07,0x87,0x07,0x87,0x07,0x87,0x07,
  0x87,0x07,0x87,0x07,0x87,0x07,0x87,0x07,0x87,0x07,0x87,0x07,0x87,0x03,0x86,0x00
};
static const unsigned char image_fill_next[] = { 
  0x06,0x40,0x06,0x60,0x0e,0x60,0x7e,0x60,0xfe,0x61,0xfe,0x61,0xfe,0x63,0xfe,0x63,
  0xfe,0x61,0x7e,0x60,0x7e,0x60,0x1e,0x60,0x0f,0x60,0x01,0x20
};

// [Answer Book]
extern const unsigned char image_book_bits[]; 

const char* weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
bool colon_switch = false;
unsigned long lastBlinkTime = 0;
long lastShiftTime = 0;
const long shiftInterval = 60000; 
int x_offset = 0;
int y_offset = 0;
unsigned long nextMoodSwitchTime = 0; // Next mood switch timestamp
bool isManualMood = false;
AppMode lastMode = MODE_EYES;

void drawClockPage() {
    display.clearDisplay(); 
    if (millis() - lastShiftTime > shiftInterval) { 
        x_offset = random(0, 3); y_offset = random(0, 3); lastShiftTime = millis(); 
    }
    
    char sDate[6]; sprintf(sDate, "%02d/%02d", currentDateTime.month(), currentDateTime.day());
    char sYear[5]; sprintf(sYear, "%d", currentDateTime.year());
    const char* sDay = weekDays[currentDateTime.dayOfTheWeek()];
    
    int rawHour = currentDateTime.hour();
    int h12 = rawHour;
    if (h12 == 0) h12 = 12; else if (h12 > 12) h12 -= 12;
    
    char sHour[3]; sprintf(sHour, "%02d", h12);
    char sMin[3]; sprintf(sMin, "%02d", currentDateTime.minute());
    
    display.setTextColor(SH110X_WHITE);

    display.setFont(&Ubuntu_Bold24pt7b);
    int currentX = 15 + x_offset;
    int BaseY = 49 + y_offset;

    // Draw hour digits
    display.setCursor(currentX, BaseY); 
    display.print(sHour);
    
    currentX += getAdafruitTextWidth(sHour);
    int spaceW = getAdafruitTextWidth(" ");
    currentX += spaceW;

    // Draw blinking colon
    if (millis() % 1000 < 500) {
        display.setCursor(currentX, BaseY - 2); 
        display.print(":");
    }

    currentX += getAdafruitTextWidth(":"); 
    currentX += spaceW;

    // Minutes blink only during time-setting mode
    // Normal mode -> always visible
    // Time-setting mode -> blink every 500ms
    if (!isTimeSetting || (millis() % 500 < 250)) {
        display.setCursor(currentX, BaseY); 
        display.print(sMin);
    }
    
    // Draw date and year
    display.setFont(&Ubuntu_Bold10pt7b); 
    display.setCursor(56 + x_offset, 13 + y_offset); display.print(sDay);
    display.setCursor(99 + x_offset, 13 + y_offset); display.print(sYear);
    display.setCursor(5 + x_offset, 13 + y_offset); display.print(sDate);
    
    display.display();
}

// 2. Draw Music Page (auto-centered track name)
void drawMusicPage() {
    display.clearDisplay(); 
    display.setTextColor(SH110X_WHITE);

    // --- State: Volume adjustment ---
    if (isVolumeAdjusting) {
        display.setFont(&Ubuntu_Bold10pt7b);
        display.setCursor(35, 20);
        display.print("VOLUME: ");
        display.print(currentVolume);

        int barMaxWidth = 100;
        int barHeight = 8;
        int barX = (128 - barMaxWidth) / 2;
        int barY = 30;

        display.drawRect(barX, barY, barMaxWidth, barHeight, SH110X_WHITE);
        int fillW = map(currentVolume, 0, 30, 0, barMaxWidth - 4); 
        if (fillW > 0) {
            display.fillRect(barX + 2, barY + 2, fillW, barHeight - 4, SH110X_WHITE);
        }
        display.setCursor(20, 55);
        display.print("[R] Set Volume");
    } 
    // --- State: Normal menu ---
    else {
        display.drawXBitmap(4, 3, image_Volup_bits, 8, 6, SH110X_WHITE);

        display.setFont(&Ubuntu_Bold14pt7b); 
        String trackInfo;
        
        // Use pgm_read_ptr for PROGMEM array access
        if (currentMusicTrack >= 1 && currentMusicTrack <= totalTracks) {
            trackInfo = String((const char*)pgm_read_ptr(&composerNames[currentMusicTrack - 1]));
        } else {
            trackInfo = "Track " + String(currentMusicTrack);
        }
        
        // Calculate centering
        int titleW = getAdafruitTextWidth(trackInfo.c_str());
        display.setCursor((128 - titleW) / 2, 33); 
        display.print(trackInfo);

        // int titleX = (128 - titleW) / 2;
        // display.setCursor(titleX, 33); // 高度維持 33
        // display.print(trackInfo);

        display.drawXBitmap(12, 41, image_music_prev_bits, 16, 16, SH110X_WHITE);
        display.drawXBitmap(102, 41, image_music_next_bits, 16, 16, SH110X_WHITE);

        // Play/Pause button (base layer)
        if (isMusicPlaying) {
             display.drawXBitmap(58, 42, image_music_pause_bits, 12, 16, SH110X_WHITE);
        } else {
             display.drawXBitmap(59, 41, image_music_play_bits, 15, 16, SH110X_WHITE);
        }

        // 2. Draw selection fill layer
        switch (musicSelection) {
            case SEL_PREV: 
                display.drawXBitmap(13, 42, image_fill_prev, 15, 14, SH110X_WHITE);
                break;
                
            case SEL_PLAY: 
                if (isMusicPlaying) {
                    // Draw solid pause fill
                    display.drawXBitmap(59, 42, image_fill_pause, 11, 15, SH110X_WHITE);
                } else {
                    // Draw solid play fill
                    display.drawXBitmap(59, 42, image_fill_play, 13, 13, SH110X_WHITE);
                }
                break;
                
            case SEL_NEXT: 
                display.drawXBitmap(102, 42, image_fill_next, 15, 14, SH110X_WHITE);
                break;
                
            case SEL_VOL: 
                display.drawRect(1, 1, 15, 10, SH110X_WHITE);
                break;
        }
    }

    display.display();
}

// 3. Draw Pomodoro Setup
void drawPomodoroSetup() {
    display.clearDisplay(); 
    display.setTextColor(SH110X_WHITE);

    String labelStr;
    int value;
    if (pomoState == POMO_SETUP_WORK) { labelStr = "Focus Time"; value = pomoWorkTime; } 
    else if (pomoState == POMO_SETUP_SHORT) { labelStr = "Short Break"; value = pomoShortBreak; } 
    else { labelStr = "Long Break"; value = pomoLongBreak; }

    display.setFont(&Ubuntu_Bold10pt7b);
    display.setCursor(33, 12);
    display.print("Pomodoro");
    display.drawLine(21, 15, 103, 15, SH110X_WHITE);

    display.setFont(&Ubuntu_Bold10pt7b);
    int labelX = 7;
    display.setCursor(labelX, 40);
    display.print(labelStr.c_str());
    
    int labelW = getAdafruitTextWidth(labelStr.c_str());
    int labelEndX = labelX + labelW;
    int minX = 102;
    
    display.setFont(&Ubuntu_Bold18pt7b);
    String valStr = String(value);
    int valW = getAdafruitTextWidth(valStr.c_str());
    int centerPoint = (labelEndX + minX) / 2;
    int valX = centerPoint - (valW / 2);
    
    display.setCursor(valX, 42); display.print(valStr);

    display.setFont(&Ubuntu_Bold10pt7b); 
    display.setCursor(minX, 40); display.print("min");
    display.setCursor(95, 63); display.print("[R]"); 
    display.setCursor(15, 63); display.print("[L]");

    display.drawXBitmap(116, 57, image_arrow_curved_right_down_bits, 8, 5, SH110X_WHITE);
    display.drawXBitmap(3, 57, image_arrow_curved_left_down_bits, 8, 5, SH110X_WHITE);

    display.display();
}

// 4. Draw Pomodoro Running
void drawPomodoroRunning() {
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setFont(&Ubuntu_Bold10pt7b); 
    display.setCursor(4, 15);
    if (pomoState == POMO_RUNNING) display.print("WORK");
    else if (pomoState == POMO_PAUSED) display.print("PAUSE");
    else display.print("BREAK");

    int mins = pomoTimerSeconds / 60;
    int secs = pomoTimerSeconds % 60;
    char timeStr[6]; sprintf(timeStr, "%02d:%02d", mins, secs);

    display.setFont(&Ubuntu_Bold24pt7b); 
    int timeW = getAdafruitTextWidth(timeStr);
    int timeX = (128 - timeW) / 2;
    display.setCursor(timeX, 45); 
    display.print(timeStr);

    int barW = 120; int barH = 10; int barX = 4; int barY = 54; 
    float progress = 0.0;
    if (pomoTotalDuration > 0) {
        progress = 1.0 - ((float)pomoTimerSeconds / (float)pomoTotalDuration);
    }
    int fillW = (int)(barW * progress);
    if (fillW > barW) fillW = barW;
    int ptrX = barX + fillW; 
    if (ptrX < barX) ptrX = barX;
    if (ptrX > barX + barW) ptrX = barX + barW;
    display.fillTriangle(ptrX - 3, barY - 5, ptrX + 3, barY - 5, ptrX, barY - 2, SH110X_WHITE);
    display.drawRect(barX, barY, barW, barH, SH110X_WHITE);
    if (fillW > 0) display.fillRect(barX, barY, fillW, barH, SH110X_WHITE);
    for (int i = 0; i < barW; i += 4) { 
        int tickX = barX + i;
        int tickH = (i % 8 == 0) ? 6 : 3; 
        uint16_t color;
        if (i < fillW) color = SH110X_BLACK; else color = SH110X_WHITE;
        display.drawFastVLine(tickX, barY + barH - tickH, tickH, color);
    }
    display.display();
}

// 5. Draw Answer Page (with slot machine animation)
void drawAnswerPage() {
    display.clearDisplay();
    u8g2_gfx.setForegroundColor(SH110X_WHITE);
    u8g2_gfx.setFontMode(1); 

    // --- State: Idle (show book icon) ---
    if (!isAnswerRevealed && !isAnswerSpinning) {
        u8g2_gfx.setFont(u8g2_font_6x10_tr);
        u8g2_gfx.drawStr(19, 11, "Book of Answers");
        u8g2_gfx.drawStr(37, 62, "Press [R]");
        display.drawXBitmap(33, 13, image_book_bits, 59, 40, SH110X_WHITE);
    } 
    // --- State: Spinning (slot machine reel) ---
    else if (isAnswerSpinning) {
        unsigned long elapsed = millis() - answerSpinStartTime;
        const unsigned long totalDuration = 2500; // 2.5 seconds total

        // Quadratic ease-out deceleration
        float progress = (float)elapsed / (float)totalDuration;
        if (progress > 1.0f) progress = 1.0f;
        float easedSpeed = 12.0f * (1.0f - progress) * (1.0f - progress);

        // Advance the reel offset
        answerSpinOffset += easedSpeed;

        // When offset exceeds line height, shift to next answer
        const int lineHeight = 20;
        while (answerSpinOffset >= lineHeight) {
            answerSpinOffset -= lineHeight;
            // Shift all reel indices up by one
            for (int i = 0; i < 4; i++) {
                answerReelIndices[i] = answerReelIndices[i + 1];
            }
            // Pick a new random entry for the bottom slot
            answerReelIndices[4] = random(0, answers_count);
        }

        // Draw title
        u8g2_gfx.setFont(u8g2_font_6x10_tr);
        u8g2_gfx.drawStr(19, 11, "Book of Answers");

        // Draw separator line
        display.drawLine(0, 14, 127, 14, SH110X_WHITE);

        // Draw visible reel entries (clipped to display area 16..63)
        u8g2_gfx.setFont(u8g2_font_ncenB10_tr);
        int yBase = 30 - (int)answerSpinOffset;
        for (int i = 0; i < 5; i++) {
            int y = yBase + i * lineHeight;
            if (y < 10 || y > 70) continue; // skip off-screen entries
            const char* text = (const char*)pgm_read_ptr(&answers_pool[answerReelIndices[i]]);
            int w = u8g2_gfx.getUTF8Width(text);
            u8g2_gfx.setCursor((128 - w) / 2, y);
            u8g2_gfx.print(text);
        }

        // Draw focus bracket in center
        display.drawRect(2, 24, 124, 18, SH110X_WHITE);

        // Animation complete: snap to final answer
        if (elapsed >= totalDuration) {
            isAnswerSpinning = false;
            isAnswerRevealed = true;
            currentAnswer = (const char*)pgm_read_ptr(&answers_pool[answerReelIndices[2]]);
        }
    }
    // --- State: Revealed (show final answer) ---
    else {
        u8g2_gfx.setFont(u8g2_font_ncenB12_tr); 
        String str = String(currentAnswer);
        int totalW = u8g2_gfx.getUTF8Width(str.c_str());
        if (totalW <= 120) {
            int x = (128 - totalW) / 2;
            u8g2_gfx.setCursor(x, 40); u8g2_gfx.print(str);
        } else {
            int splitIndex = -1;
            for (int i = 0; i < (int)str.length(); i++) {
                if (str[i] == ' ') {
                    String sub = str.substring(0, i);
                    if (u8g2_gfx.getUTF8Width(sub.c_str()) < 124) splitIndex = i; else break;
                }
            }
            if (splitIndex == -1) splitIndex = str.lastIndexOf(' ');
            String line1 = str.substring(0, splitIndex);
            String line2 = str.substring(splitIndex + 1);
            int w1 = u8g2_gfx.getUTF8Width(line1.c_str());
            u8g2_gfx.setCursor((128 - w1) / 2, 32); u8g2_gfx.print(line1);
            int w2 = u8g2_gfx.getUTF8Width(line2.c_str());
            u8g2_gfx.setCursor((128 - w2) / 2, 52); u8g2_gfx.print(line2);
        }
        u8g2_gfx.setFont(u8g2_font_6x10_tr);
        u8g2_gfx.drawStr(15, 63, "[L]");
        display.drawXBitmap(3, 57, image_arrow_curved_left_down_bits, 8, 5, SH110X_WHITE);
    }
    display.display();
}

void TaskUICode(void * parameter) {
    if(!display.begin(0, true)) { for(;;); }
    u8g2_gfx.begin(display); 
    display.clearDisplay(); display.display();

    roboEyes.begin(128, 64, 60); 
    roboEyes.setHeight(40, 40); roboEyes.setWidth(30, 30); roboEyes.setSpacebetween(20); roboEyes.setBorderradius(20, 20);
    roboEyes.setAutoblinker(true, 4, 2); roboEyes.setCuriosity(true); roboEyes.setIdleMode(true, 1, 4);
    
    lastMode = currentMode;

    for(;;) {

        // --- Handle OLED off/on during sleep mode ---
        static bool lastAsleepState = false;

        if (isAsleep) {
            if (!lastAsleepState) {
                // Turn off display to prevent OLED burn-in and maximize power saving
                display.clearDisplay();
                display.display();
                
                // Adafruit_SH1106G may support this command to disable panel driver
                // (If compile error, comment out - clearDisplay alone still saves power)
                // display.enableDisplay(false); 
                
                lastAsleepState = true;
            }
            vTaskDelay(pdMS_TO_TICKS(100)); // Slow down UI task during sleep to save resources
            continue; // Important: skip all drawing logic below
        } else if (lastAsleepState) {
            // Just woke up, re-enable panel
            // display.enableDisplay(true); 
            lastAsleepState = false;
            isUpdateRequired = true; // Force redraw current mode
        }

        if (isBrightnessChanged) {
              display.setContrast(displayBrightness); // Execute SPI command from UI task core
              isBrightnessChanged = false;
        }
        if (currentMode != lastMode) {
            display.clearDisplay();
            if (currentMode == MODE_EYES) {
                roboEyes.setHeight(35, 35); roboEyes.setCuriosity(true); roboEyes.setIdleMode(true, 1, 2); roboEyes.setMood(DEFAULT);
            }
            lastMode = currentMode;
        }

        switch (currentMode) {
            case MODE_EYES:
                if (isBotTired) {
                    // --- Tired mode logic ---
                    roboEyes.setMood(TIRED);       
                    roboEyes.setCuriosity(false);    
                    roboEyes.setIdleMode(false);     
                    roboEyes.setPosition(S);         
                } 
                else {
                    // --- Default mode: 10% mutation chance ---
                    if (millis() >= nextMoodSwitchTime) {
                        nextMoodSwitchTime = millis() + random(3000, 10000);
                        
                        // [Key] Reset to normal mode parameters and clear flicker
                        roboEyes.setHeight(35, 35); 
                        roboEyes.setWidth(30, 30);
                        roboEyes.setBorderradius(20, 20);
                        roboEyes.setHFlicker(false);  // Clear any residual shake flicker
                        roboEyes.setMood(DEFAULT);

                        // Random 0~99, <10 means 10% probability
                        if (random(100) < 10) {
                            // Enter mutation mode: randomly pick a variation
                            int mutation = random(2);
                            if (mutation == 0) {
                                roboEyes.setBorderradius(4, 4); // Square eyes
                            } else {
                                roboEyes.setWidth(12, 12);      // Narrow squinting eyes
                            }
                        } else {
                            // 90% chance: stay in normal mode
                            // Occasionally show happy expression
                            if (random(100) > 85) {
                                roboEyes.setMood(HAPPY);
                            }
                        }
                        
                        // Ensure curiosity and idle mode are on in normal mode
                        roboEyes.setCuriosity(true);
                        roboEyes.setIdleMode(true);
                    }
                }
                roboEyes.update();
                break;
            case MODE_CLOCK:
                drawClockPage();
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            case MODE_MUSIC:
                if (isUpdateRequired) { drawMusicPage(); isUpdateRequired = false; }
                break;
            case MODE_POMODORO:
                if (pomoState == POMO_SETUP_WORK || pomoState == POMO_SETUP_SHORT || pomoState == POMO_SETUP_LONG) {
                    if (isUpdateRequired) { drawPomodoroSetup(); isUpdateRequired = false; }
                } else {
                    if (isUpdateRequired) { drawPomodoroRunning(); isUpdateRequired = false; }
                }
                break;
            case MODE_ANSWER:
                if (isAnswerSpinning) {
                    drawAnswerPage(); // Continuous rendering during animation
                } else if (isUpdateRequired) {
                    drawAnswerPage();
                    isUpdateRequired = false;
                }
                break;    
        }
        vTaskDelay(pdMS_TO_TICKS(16)); 
    }
}