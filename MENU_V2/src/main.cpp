#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define WIFI_SSID "VM0459056"
#define WIFI_PASSWORD "p6zTqmm6vxqc"
#define SERVER_URL "http://192.168.0.27:5000"

void connectWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
}

// Function to mark a reminder as completed
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
            Serial.println("Task marked as completed: " + task);
        } else {
            Serial.println("Failed to update task. HTTP Response Code: " + String(httpResponseCode));
        }
        http.end();
    }
}

// Function to get reminders from the server
void getReminders() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(String(SERVER_URL) + "/get_reminders");
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200) {
            String response = http.getString();
            Serial.println("Reminders: " + response);

            DynamicJsonDocument doc(1024);
            deserializeJson(doc, response);

            for (JsonVariant reminder : doc.as<JsonArray>()) {
                String task = reminder["task"];
                String status = reminder["status"];

                if (status == "pending") {
                    Serial.println("Pending Reminder: " + task);
                }
            }
        } else {
            Serial.println("Error fetching reminders. HTTP Response Code: " + String(httpResponseCode));
        }
        http.end();
    }
}

// Function to read user input from serial monitor and mark the task as complete
void readSerialInput() {
    if (Serial.available() > 0) {
        String input = Serial.readStringUntil('\n'); // Read input until newline
        input.trim(); // Remove any trailing newline or spaces

        if (input.startsWith("done ")) {
            String task = input.substring(5); // Extract task name after "done "
            if (task.length() > 0) {
                Serial.println("User input received: Marking task as complete -> " + task);
                completeReminder(task);
            } else {
                Serial.println("Error: No task name provided.");
            }
        } else {
            Serial.println("Invalid input format. Use: done TASK_NAME");
        }
    }
}

void setup() {
    Serial.begin(115200);
    connectWiFi();
}

void loop() {
    getReminders();
    readSerialInput();  // Check for user input from serial
    delay(5000);  // Check every 5 seconds
}
