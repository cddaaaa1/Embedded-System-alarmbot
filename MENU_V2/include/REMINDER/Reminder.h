#ifndef REMINDER_H
#define REMINDER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "SPIFFS.h"

// WiFi Credentials
#define WIFI_SSID "VM0459056"
#define WIFI_PASSWORD "p6zTqmm6vxqc"

// Server URL
#define SERVER_URL "http://192.168.0.27:5000"

// JSON File Path
#define TODO_FILE "/todo.json"

// Function Prototypes
void connectWiFi();
void completeReminder(String task);
void getReminders();
void readSerialInput();
void saveTodosToFile(DynamicJsonDocument &doc);
void loadTodosFromFile(DynamicJsonDocument &doc);
void syncCompletedTodos();
void toggleTodoStateByName(String taskName);


#endif // REMINDER_H
