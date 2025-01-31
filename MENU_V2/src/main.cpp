#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "MENU/StateMachine.h"
#include "MENU/DisplayManager.h"

#include <WiFi.h>      // ESP32 Wi-Fi库
#include <time.h>      // time.h 库 (ESP32内置)
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#define WIFI_SSID "VM0459056"
#define WIFI_PASSWORD "p6zTqmm6vxqc"
#define SERVER_URL "http://192.168.0.27:5000"
#define WEBSOCKET_URL "ws://192.168.0.27:8080"  // WebSocket URL

using namespace websockets;

WebsocketsClient client;

void connectWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
}

// Function to notify the server that a task is completed
void completeReminder(String task) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(String(SERVER_URL) + "/update_reminder");
        http.addHeader("Content-Type", "application/json");

        // Create JSON payload
        DynamicJsonDocument doc(200);
        doc["task"] = task;
        String payload;
        serializeJson(doc, payload);

        int httpResponseCode = http.POST(payload);
        if (httpResponseCode == 200) {
            Serial.println("Task completed: " + task);
        } else {
            Serial.println("Failed to update task. HTTP Response Code: " + String(httpResponseCode));
        }
        http.end();
    }
}

// WebSocket event handler
void onMessage(WebsocketsMessage message) {
    Serial.println("New Reminder Received: " + message.data());

    // Parse JSON reminder
    DynamicJsonDocument doc(200);
    deserializeJson(doc, message.data());
    String task = doc["task"];

    // Simulate executing the task
    Serial.println("Executing Task: " + task);
    delay(5000);  // Simulate execution

    // Notify server task is completed
    completeReminder(task);
}

// Connect WebSocket
void connectWebSocket() {
    Serial.print("🔄 Attempting WebSocket connection to: ");
    Serial.println(WEBSOCKET_URL);

    if (client.connect(WEBSOCKET_URL)) {
        Serial.println("✅ WebSocket Connected!");
        client.onMessage(onMessage);
    } else {
        Serial.println("❌ WebSocket Connection Failed. Retrying in 5s...");
        delay(5000);
        connectWebSocket();
    }
}

void setup() {
    Serial.begin(115200);
    connectWiFi();
    connectWebSocket();
}

void loop() {
    if (client.available()) {
        client.poll();
    }
}
