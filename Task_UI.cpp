#include "Globals.h"
#include <U8g2_for_Adafruit_GFX.h>

void TaskUICode(void * parameter);
U8G2_FOR_ADAFRUIT_GFX u8g2_gfx;

// --- Bitmap Resources (XBM) ---

// 1. [保留] 箭頭資源
static const unsigned char image_arrow_curved_left_down_bits[] = {0x18,0x7d,0x8f,0x07,0x0f};
static const unsigned char image_arrow_curved_right_down_bits[] = {0x18,0xbe,0xf1,0xe0,0xf0};

// 2. [音樂播放器] 基礎空心圖示 (Base Icons)
static const unsigned char image_music_prev_bits[] = {
  0x00,0x00,0x0f,0xe0,0x09,0x90,0x09,0x8c,0x09,0x82,0x89,0x81,0x49,0x80,0x39,0x80,
  0x39,0x80,0x49,0x80,0x89,0x81,0x09,0x82,0x09,0x8c,0x09,0x90,0x0f,0xe0,0x00,0x00
};
static const unsigned char image_music_play_bits[] = {
  0x03,0x00,0x07,0x00,0x19,0x00,0x61,0x00,0x81,0x01,0x01,0x06,0x01,0x18,0x01,0x60,
  0x01,0x18,0x01,0x06,0x81,0x01,0x61,0x00,0x19,0x00,0x07,0x00,0x03,0x00,0x00,0x00
};
// 暫停鍵底圖 (12x16)
static const unsigned char image_music_pause_bits[] = {
  0x9f,0x0f,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,
  0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x91,0x08,0x9f,0x0f,0x00,0x00
};
static const unsigned char image_music_next_bits[] = {
  0x00,0x00,0x07,0xf0,0x09,0x90,0x31,0x90,0x41,0x90,0x81,0x91,0x01,0x92,0x01,0x9c,
  0x01,0x9c,0x01,0x92,0x81,0x91,0x41,0x90,0x31,0x90,0x09,0x90,0x07,0xf0,0x00,0x00
};
static const unsigned char image_Volup_bits[] = {0x48,0x8c,0xaf,0xaf,0x8c,0x48};

// 3. [音樂播放器] 填充圖層 (Fill Layers)
static const unsigned char image_fill_prev[] = { 
  0x00,0x20,0x03,0x30,0x03,0x38,0x03,0x3e,0x03,0x3f,0xc3,0x3f,0xe3,0x3f,0xe3,0x7f,
  0xc3,0x7f,0x03,0x7f,0x03,0x7e,0x03,0x7a,0x03,0x70,0x00,0x40
};
static const unsigned char image_fill_play[] = { 
  0x04,0x00,0x07,0x00,0x1e,0x00,0x7e,0x00,0xfe,0x05,0xfe,0x0f,0xfe,0x1f,0xfe,0x07,
  0xfe,0x07,0xfe,0x00,0x5e,0x00,0x1e,0x00,0x04,0x00
};
// 暫停鍵填充圖層 (11x15)
static const unsigned char image_fill_pause[] = {
  0x02,0x01,0x87,0x03,0x87,0x03,0x87,0x03,0x87,0x07,0x87,0x07,0x87,0x07,0x87,0x07,
  0x87,0x07,0x87,0x07,0x87,0x07,0x87,0x07,0x87,0x07,0x87,0x07,0x87,0x03,0x86,0x00
};
static const unsigned char image_fill_next[] = { 
  0x06,0x40,0x06,0x60,0x0e,0x60,0x7e,0x60,0xfe,0x61,0xfe,0x61,0xfe,0x63,0xfe,0x63,
  0xfe,0x61,0x7e,0x60,0x7e,0x60,0x1e,0x60,0x0f,0x60,0x01,0x20
};

// [解答之書]
extern const unsigned char image_book_bits[]; 

const char* weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
bool colon_switch = false;
unsigned long lastBlinkTime = 0;
long lastShiftTime = 0;
const long shiftInterval = 60000; 
int x_offset = 0;
int y_offset = 0;
AppMode lastMode = MODE_EYES;

// 1. Draw Clock
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
    
    u8g2_gfx.setForegroundColor(SH110X_WHITE);
    u8g2_gfx.setBackgroundColor(SH110X_BLACK);
    u8g2_gfx.setFontMode(1); 

    u8g2_gfx.setFont(u8g2_font_profont29_tr);
    int currentX = 15 + x_offset;
    int BaseY = 49 + y_offset;
    u8g2_gfx.setCursor(currentX, BaseY); u8g2_gfx.print(sHour);
    currentX += u8g2_gfx.getUTF8Width(sHour);
    int spaceW = u8g2_gfx.getUTF8Width(" ");
    currentX += spaceW;
    if (millis() % 1000 < 500) {
        u8g2_gfx.setCursor(currentX, BaseY - 2); u8g2_gfx.print(":");
    }
    currentX += u8g2_gfx.getUTF8Width(":"); currentX += spaceW;
    u8g2_gfx.setCursor(currentX, BaseY); u8g2_gfx.print(sMin);
    u8g2_gfx.setFont(u8g2_font_t0_12b_tr); u8g2_gfx.setCursor(56 + x_offset, 13 + y_offset); u8g2_gfx.print(sDay);
    u8g2_gfx.setCursor(99 + x_offset, 13 + y_offset); u8g2_gfx.print(sYear);
    u8g2_gfx.setCursor(5 + x_offset, 13 + y_offset); u8g2_gfx.print(sDate);
    display.display();
}

// 2. [修改] Draw Music Page (自動置中歌名)
void drawMusicPage() {
    display.clearDisplay(); 
    u8g2_gfx.setForegroundColor(SH110X_WHITE);
    u8g2_gfx.setFontMode(1);

    // --- 狀態：正在調整音量 ---
    if (isVolumeAdjusting) {
        u8g2_gfx.setFont(u8g2_font_6x10_tr);
        u8g2_gfx.setCursor(35, 20);
        u8g2_gfx.print("VOLUME: ");
        u8g2_gfx.print(currentVolume);

        int barMaxWidth = 100;
        int barHeight = 8;
        int barX = (128 - barMaxWidth) / 2;
        int barY = 30;

        display.drawRect(barX, barY, barMaxWidth, barHeight, SH110X_WHITE);
        int fillW = map(currentVolume, 0, 30, 0, barMaxWidth - 4); 
        if (fillW > 0) {
            display.fillRect(barX + 2, barY + 2, fillW, barHeight - 4, SH110X_WHITE);
        }
        u8g2_gfx.setCursor(20, 55);
        u8g2_gfx.print("[R] Set Volume");
    } 
    // --- 狀態：正常選單 ---
    else {
        // 1. 繪製基礎圖層
        display.drawXBitmap(4, 3, image_Volup_bits, 8, 6, SH110X_WHITE);

        // [關鍵修改] 歌名顯示邏輯
        u8g2_gfx.setFont(u8g2_font_timR14_tr); 
        String trackInfo = "Track " + String(currentMusicTrack);
        
        // 計算文字寬度並置中
        int titleW = u8g2_gfx.getUTF8Width(trackInfo.c_str());
        int titleX = (128 - titleW) / 2;
        
        u8g2_gfx.setCursor(titleX, 33); // 高度維持 33
        u8g2_gfx.print(trackInfo);

        display.drawXBitmap(12, 41, image_music_prev_bits, 16, 16, SH110X_WHITE);
        display.drawXBitmap(102, 41, image_music_next_bits, 16, 16, SH110X_WHITE);

        // 播放/暫停按鈕 (底圖)
        if (isMusicPlaying) {
             display.drawXBitmap(58, 42, image_music_pause_bits, 12, 16, SH110X_WHITE);
        } else {
             display.drawXBitmap(59, 41, image_music_play_bits, 15, 16, SH110X_WHITE);
        }

        // 2. 繪製選取填充層
        switch (musicSelection) {
            case SEL_PREV: 
                display.drawXBitmap(13, 42, image_fill_prev, 15, 14, SH110X_WHITE);
                break;
                
            case SEL_PLAY: 
                if (isMusicPlaying) {
                    // 畫實心暫停填充
                    display.drawXBitmap(59, 42, image_fill_pause, 11, 15, SH110X_WHITE);
                } else {
                    // 畫實心播放填充
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
    u8g2_gfx.setForegroundColor(SH110X_WHITE);
    u8g2_gfx.setFontMode(1);

    String labelStr;
    int value;
    if (pomoState == POMO_SETUP_WORK) { labelStr = "Focus Time"; value = pomoWorkTime; } 
    else if (pomoState == POMO_SETUP_SHORT) { labelStr = "Short Break"; value = pomoShortBreak; } 
    else { labelStr = "Long Break"; value = pomoLongBreak; }

    u8g2_gfx.setFont(u8g2_font_t0_12b_tr); u8g2_gfx.drawStr(39, 12, "Pomodoro");
    display.drawLine(21, 15, 103, 15, SH110X_WHITE);

    u8g2_gfx.setFont(u8g2_font_6x10_tr);
    int labelX = 7;
    u8g2_gfx.drawStr(labelX, 40, labelStr.c_str());
    
    int labelW = u8g2_gfx.getUTF8Width(labelStr.c_str());
    int labelEndX = labelX + labelW;
    int minX = 102;
    
    u8g2_gfx.setFont(u8g2_font_t0_18b_tr);
    String valStr = String(value);
    int valW = u8g2_gfx.getUTF8Width(valStr.c_str());
    int centerPoint = (labelEndX + minX) / 2;
    int valX = centerPoint - (valW / 2);
    
    u8g2_gfx.setCursor(valX, 42); u8g2_gfx.print(valStr);

    u8g2_gfx.setFont(u8g2_font_6x10_tr); u8g2_gfx.drawStr(minX, 40, "min");
    u8g2_gfx.drawStr(95, 63, "[R]"); 
    u8g2_gfx.drawStr(15, 63, "[L]");

    display.drawXBitmap(116, 57, image_arrow_curved_right_down_bits, 8, 5, SH110X_WHITE);
    display.drawXBitmap(3, 57, image_arrow_curved_left_down_bits, 8, 5, SH110X_WHITE);

    display.display();
}

// 4. Draw Pomodoro Running
void drawPomodoroRunning() {
    display.clearDisplay();
    u8g2_gfx.setForegroundColor(SH110X_WHITE);
    u8g2_gfx.setFontMode(1);
    u8g2_gfx.setFont(u8g2_font_profont12_tf); 
    u8g2_gfx.setCursor(4, 10);
    if (pomoState == POMO_RUNNING) u8g2_gfx.print("WORK");
    else if (pomoState == POMO_PAUSED) u8g2_gfx.print("PAUSE");
    else u8g2_gfx.print("BREAK");

    int mins = pomoTimerSeconds / 60;
    int secs = pomoTimerSeconds % 60;
    char timeStr[6]; sprintf(timeStr, "%02d:%02d", mins, secs);

    u8g2_gfx.setFont(u8g2_font_logisoso24_tn); 
    int timeW = u8g2_gfx.getUTF8Width(timeStr);
    int timeX = (128 - timeW) / 2;
    u8g2_gfx.setCursor(timeX, 42); 
    u8g2_gfx.print(timeStr);

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

// 5. Draw Answer Page
void drawAnswerPage() {
    display.clearDisplay();
    u8g2_gfx.setForegroundColor(SH110X_WHITE);
    u8g2_gfx.setFontMode(1); 

    if (!isAnswerRevealed) {
        u8g2_gfx.setFont(u8g2_font_6x10_tr);
        u8g2_gfx.drawStr(19, 11, "Book of Answers");
        u8g2_gfx.drawStr(37, 62, "Press [R]");
        display.drawXBitmap(33, 13, image_book_bits, 59, 40, SH110X_WHITE);
    } 
    else {
        u8g2_gfx.setFont(u8g2_font_ncenB12_tr); 
        String str = String(currentAnswer);
        int totalW = u8g2_gfx.getUTF8Width(str.c_str());
        if (totalW <= 120) {
            int x = (128 - totalW) / 2;
            u8g2_gfx.setCursor(x, 40); u8g2_gfx.print(str);
        } else {
            int splitIndex = -1;
            for (int i = 0; i < str.length(); i++) {
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
    roboEyes.setHeight(35, 35); roboEyes.setWidth(40, 40); roboEyes.setSpacebetween(20); 
    roboEyes.setAutoblinker(true, 4, 2); roboEyes.setCuriosity(true); roboEyes.setIdleMode(true, 1, 2);
    
    lastMode = currentMode;

    for(;;) {
        if (currentMode != lastMode) {
            display.clearDisplay();
            if (currentMode == MODE_EYES) {
                roboEyes.setHeight(35, 35); roboEyes.setCuriosity(true); roboEyes.setIdleMode(true, 1, 2); roboEyes.setMood(DEFAULT);
            }
            lastMode = currentMode;
        }

        switch (currentMode) {
            case MODE_EYES:
                if (isBotTired) { roboEyes.setMood(TIRED); }
                roboEyes.update();
                break;
            case MODE_CLOCK:
                drawClockPage();
                vTaskDelay(pdMS_TO_TICKS(100));
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
                if (isUpdateRequired) { drawAnswerPage(); isUpdateRequired = false; }
                break;    
        }
        vTaskDelay(pdMS_TO_TICKS(16)); 
    }
}