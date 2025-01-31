#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "MENU/StateMachine.h"
#include "MENU/DisplayManager.h"

#include <WiFi.h>      // ESP32 Wi-Fi库
#include <time.h>      // time.h 库 (ESP32内置)

#include "REMINDER/reminder.h"

// const char* ssid     = "201WI-FI";
// const char* password = "123456789";
const char* ssid     = "VM0459056";
const char* password = "p6zTqmm6vxqc";
// 2) 时区和夏令时设置
//    例如：中国一般是UTC+8，不用夏令时 => gmtOffset=8小时，daylightOffset=0
//    若你在其他时区，请自行调整。

const long gmtOffset_sec = 0;         // 英国 GMT+0
const int daylightOffset_sec = 3600; // 英国夏令时偏移为 +1 小时
// 3) NTP服务器地址，可用"pool.ntp.org"或地区服务器
const char* ntpServer = "pool.ntp.org";

int currentHour   = 0;
int currentMinute = 0;
int currentSecond = 0;

// Timer for getReminders (10 seconds interval)
unsigned long lastReminderCheck = 0;
const unsigned long reminderInterval = 10000; // 10 seconds in milliseconds

void setup() {
  Serial.begin(115200);
  // 连接WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.printf("Connecting to WiFi: %s\n", ssid);

  // 等待WiFi连上
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");

  // 使用configTime函数设置时区和NTP服务器
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // 检查是否成功获取时间
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time from NTP!");
    // 你也可以在这里重试或做其他处理
  } else {
    Serial.println("Time sync with NTP successful.");
  }

  Serial.println("=== Menu via Serial Input ===");
  Serial.println("[W/w] = Up, [S/s] = Down, [E/e] = Enter");
  Serial.println("[Space] in INIT to switch to MAIN_MENU");
  Serial.println("==================================");
  initOled();
  initFace();
  initStateMachine();
}

void loop() {
    // 声明一个 tm 结构体来接收时间
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      // 若成功获取
      currentHour   = timeinfo.tm_hour;
      currentMinute = timeinfo.tm_min;
      currentSecond = timeinfo.tm_sec;
    } else {
      Serial.println("Failed to get time, will try again...");
    }
    update(); //get input and update state
    currentState = getState();
    render(currentState);//渲染图像

    // Call getReminders() only if 10 seconds have passed
    if (millis() - lastReminderCheck >= reminderInterval) {
        lastReminderCheck = millis(); // Reset the timer
        getReminders();
    }
    readSerialInput();

}

