#include <Arduino.h>
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
    // Stack 設大一點 (6000) 以防圖形庫爆記憶體
    xTaskCreatePinnedToCore(
        TaskUICode, "TaskUI", 6000, NULL, 1, &TaskUI, 1
    );

    // 建立 Task Sensors (Core 0 - 負責硬體邏輯)
    xTaskCreatePinnedToCore(
        TaskSensorsCode, "TaskSensors", 4096, NULL, 1, &TaskSensors, 0
    );
    
    Serial.println("Tasks Created.");
}

void loop() {
    // 釋放 Arduino Loop 資源
    vTaskDelete(NULL);
}