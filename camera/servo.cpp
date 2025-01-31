#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <Arduino.h>

// -------------------------------
// 1. WiFi 配置
// -------------------------------
const char* ssid = "x7";
const char* password = "ctx20040510";

// -------------------------------
// 2. 舵机相关
// -------------------------------
#define SERVO_YAW_PIN   26
#define SERVO_PITCH_PIN 25

Servo servoYaw;
Servo servoPitch;

// 舵机角度范围
#define SERVO_MIN_ANGLE 0
#define SERVO_MAX_ANGLE 180

// 存储当前角度（初始值 90°，表示居中）
int current_yaw = 90;
int current_pitch = 90;

// -------------------------------
// 3. 处理舵机运动
// -------------------------------
void moveServo(float yaw_offset, float pitch_offset) {
    current_yaw += yaw_offset;
    current_pitch += pitch_offset;

    // 限制角度范围
    current_yaw = constrain(current_yaw, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
    current_pitch = constrain(current_pitch, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);

    servoYaw.write(current_yaw);
    servoPitch.write(current_pitch);

    Serial.printf("🎯 Moving Servos - New Yaw: %d°, New Pitch: %d°\n", current_yaw, current_pitch);
}

// -------------------------------
// 4. Web 服务器
// -------------------------------
AsyncWebServer server(80);

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi Connected!");

    // 初始化舵机
    servoYaw.attach(SERVO_YAW_PIN, 500, 2400);
    servoPitch.attach(SERVO_PITCH_PIN, 500, 2400);
    servoYaw.write(current_yaw);
    servoPitch.write(current_pitch);

    server.on("/update_angles", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            StaticJsonDocument<256> doc;
            deserializeJson(doc, data, len);
            moveServo(doc["yaw"], doc["pitch"]);
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    server.begin();
}

void loop() {}