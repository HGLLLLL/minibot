#include <Arduino.h>
#include <Preferences.h> // For persistent settings storage
#include "Config.h"
#include "Globals.h"

// External task function declarations
extern void TaskSensorsCode(void * parameter);
extern void TaskUICode(void * parameter);

TaskHandle_t TaskSensors;
TaskHandle_t TaskUI;

void setup() {
    Serial.begin(115200);
    // Wait for serial port stabilization
    delay(1000);
    Serial.println("--- Minibot OS Booting ---");

    // Create Task UI (Core 1 - handles display)
    xTaskCreatePinnedToCore(
        TaskUICode, "TaskUI", 6000, NULL, 1, &TaskUI, 1
    );

    // Create Task Sensors (Core 0 - handles hardware logic)
    xTaskCreatePinnedToCore(
        TaskSensorsCode, "TaskSensors", 4096, NULL, 1, &TaskSensors, 0
    );

    Serial.println("Tasks Created.");
    
    Preferences prefs;
    // Read saved settings (use defaults if never set before)
    prefs.begin("bot-settings", true);
    displayBrightness = prefs.getInt("bright", 150);
    currentVolume = prefs.getInt("vol", 15);      // Read volume, default 15
    pomoWorkTime = prefs.getInt("pomoW", 25);     // Read focus time, default 25
    pomoShortBreak = prefs.getInt("pomoS", 5);    // Read short break, default 5
    pomoLongBreak = prefs.getInt("pomoL", 15);    // Read long break, default 15
    prefs.end();

    // Apply brightness setting
    isBrightnessChanged = true;
}

void loop() {
    // Release Arduino Loop resources (FreeRTOS tasks handle everything)
    vTaskDelete(NULL);
}