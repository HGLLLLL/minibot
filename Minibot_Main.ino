#include <Arduino.h>
#include <Preferences.h> // <--- 1. 新增這個 include
#include "Config.h"
#include "Globals.h"

// 引入外部 Task 函數
extern void TaskSensorsCode(void * parameter);
extern void TaskUICode(void * parameter);

TaskHandle_t TaskSensors;
TaskHandle_t TaskUI;

void setup() {
    Serial.begin(115200);
    // 等待序列埠穩定
    delay(1000);
    Serial.println("--- Minibot OS Booting ---");

    // 建立 Task UI (Core 1 - 負責顯示)
    xTaskCreatePinnedToCore(
        TaskUICode, "TaskUI", 6000, NULL, 1, &TaskUI, 1
    );

    // 建立 Task Sensors (Core 0 - 負責硬體邏輯)
    xTaskCreatePinnedToCore(
        TaskSensorsCode, "TaskSensors", 4096, NULL, 1, &TaskSensors, 0
    );

    Serial.println("Tasks Created.");
    
    Preferences prefs;
    // 讀取各種設定 (如果沒設定過，就給預設值)
    prefs.begin("bot-settings", true);
    displayBrightness = prefs.getInt("bright", 150);
    currentVolume = prefs.getInt("vol", 15);      // 讀取音量，預設 20
    pomoWorkTime = prefs.getInt("pomoW", 25);     // 讀取專注時間，預設 25
    pomoShortBreak = prefs.getInt("pomoS", 5);    // 讀取短休息，預設 5
    pomoLongBreak = prefs.getInt("pomoL", 15);    // 讀取長休息，預設 15
    prefs.end();

    // 套用亮度
    isBrightnessChanged = true;
}

void loop() {
    // 釋放 Arduino Loop 資源
    vTaskDelete(NULL);
}