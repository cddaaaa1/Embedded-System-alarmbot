#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "MENU/StateMachine.h"
#include "MENU/DisplayManager.h"

#include <WiFi.h> // ESP32 Wi-Fi库
#include <time.h> // time.h 库 (ESP32内置)
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char* ssid = "VM0459056";
const char* password = "p6zTqmm6vxqc";
const char* serverIP = "192.168.0.27"; // Replace with your PC's local IP
const int serverPort = 8765;            // WebSocket server port

WebSocketsClient webSocket;
String reminders[10]; // Store up to 10 reminders
int taskCount = 0;    // Track active reminders
bool isConnected = false;

void parseReminders(String jsonResponse)
{
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, jsonResponse);
    JsonArray reminderArray = doc["reminders"];

    taskCount = 0;
    Serial.println("Pending Reminders:");
    for (JsonVariant task : reminderArray)
    {
        if (taskCount < 10)
        {
            reminders[taskCount] = task.as<String>();
            Serial.println("- " + reminders[taskCount]);
            taskCount++;
        }
    }
    Serial.println("Type 'done task_name' to complete a reminder.");
}

void completeReminder(String task)
{
    DynamicJsonDocument doc(256);
    doc["done"] = task;

    String requestBody;
    serializeJson(doc, requestBody);
    webSocket.sendTXT(requestBody); // Send completion message to the server

    Serial.println("Task marked as completed: " + task);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_TEXT:
        Serial.println("New reminder update received!");
        parseReminders((char *)payload);
        break;
    case WStype_DISCONNECTED:
        Serial.println("WebSocket Disconnected");
        break;
    case WStype_CONNECTED:
        Serial.println("WebSocket Connected to Server");
        break;
    }
}


void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    webSocket.begin(serverIP,serverPort, "/");
    webSocket.onEvent(webSocketEvent);
}


void loop()
{
    webSocket.loop();

    if (Serial.available() > 0)
    {
        String input = Serial.readStringUntil('\n');
        input.trim(); // Remove whitespace

        if (input.startsWith("done "))
        {
            String task = input.substring(5); // Extract task name
            completeReminder(task);
        }
        else
        {
            Serial.println("Invalid input. Type 'done task_name' to complete a reminder.");
        }
    }

    delay(100);
}
