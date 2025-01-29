#include "MENU/ClockSetting.h"
#include <Arduino.h>

AlarmTime alarmTime;
bool alarmOn = false;
const int potentiometerPin = 35;  // 电位器接到模拟引脚 A0
int lastPotValue = 0;            // 上一次电位器值
int currentPotValue = 0;         // 当前电位器值
int threshold = 10;              // 变化阈值，避免过于灵敏



// 在这里定义(全局作用域)
AlarmTime countdownInitialTime;
AlarmTime countdownTime;
bool countdownStopped = false;

// =========== 定时闹钟新增的全局 ==========
AlarmTime timedAlarmTime;
bool timedAlarmTriggered = false;

void initAlarmTime() {
    alarmTime.hour = 0;
    alarmTime.minute = 0;
    alarmTime.second = 0;
    alarmOn = false;
    pinMode(potentiometerPin, INPUT);
}

void increaseHour() {
    alarmTime.hour = (alarmTime.hour + 1) % 24;
}
void decreaseHour() {
    alarmTime.hour = (alarmTime.hour + 23) % 24;
}
void increaseMinute() {
    alarmTime.minute = (alarmTime.minute + 1) % 60;
}
void decreaseMinute() {
    alarmTime.minute = (alarmTime.minute + 59) % 60;
}
void increaseSecond() {
    alarmTime.second = (alarmTime.second + 1) % 60;
}
void decreaseSecond() {
    alarmTime.second = (alarmTime.second + 59) % 60;
}

void toggleAlarmOnOff(){
    alarmOn = !alarmOn;
}


// ========== 倒计时相关 ==========
// 当用户在 ALARM_MENU 里“Confirm”后调用它
void initCountdown() {
    // countdownInitialTime 复制自 alarmTime
    countdownInitialTime = alarmTime;
    // countdownTime = initialTime
    countdownTime = countdownInitialTime;

    // 默认“未暂停”
    countdownStopped = false;
}

// 重置到初始
void resetCountdown() {
    countdownTime = countdownInitialTime;
    countdownStopped = false;
}

// 每秒调用一次
bool countdownTickOneSecond() {
    // 如果暂停了，就不动
    if(countdownStopped) return false;

    // 把 countdownTime 减 1秒
    int totalSec = countdownTime.hour * 3600 
                 + countdownTime.minute * 60
                 + countdownTime.second;

    if(totalSec > 0){
        totalSec--;
        // 更新回 countdownTime
        countdownTime.hour   = totalSec / 3600;
        countdownTime.minute = (totalSec % 3600) / 60;
        countdownTime.second = totalSec % 60;

        // 未到 0
        return false;
    } else {
        // 已经是0 或减到0
        return true;
    }
}

// 是否剩余时间为0
bool isCountdownZero(){
    return (countdownTime.hour == 0 
         && countdownTime.minute == 0 
         && countdownTime.second == 0);
}



// ========== 定时闹钟部分 ==========
// initAlarm: 将用户编辑的 alarmTime 保存到 timedAlarmTime
void initAlarm(){
    timedAlarmTime = alarmTime;  // 复制
    timedAlarmTriggered = false; // 重置标志
    Serial.printf("initAlarm => timedAlarmTime = %02d:%02d:%02d\n",
        timedAlarmTime.hour, timedAlarmTime.minute, timedAlarmTime.second);
}

// checkAlarm: 对比“系统当前时间”是否 >= timedAlarmTime
// 如果到达且 alarmOn==true，则返回 true 以触发响铃
bool checkAlarm(){
    if (!alarmOn) return false;    // 如果闹钟没开，就不触发

    // 这里假设有个 “系统时间 currentHour/currentMinute/currentSecond”
    // 你需要自己实现或在别处维护
    extern int currentHour;
    extern int currentMinute;
    extern int currentSecond;

    // 如果已经触发过就不重复触发
    if (timedAlarmTriggered) return false;

    // 简单判断：如果当前时分秒 >= timedAlarmTime，就触发
    // 你也可以更精确地比较
    if ( (currentHour > timedAlarmTime.hour) ||
         (currentHour == timedAlarmTime.hour && currentMinute > timedAlarmTime.minute) ||
         (currentHour == timedAlarmTime.hour && currentMinute == timedAlarmTime.minute
           && currentSecond >= timedAlarmTime.second) )
    {
        timedAlarmTriggered = true;
        Serial.println("=== Timed Alarm Triggered! ===");
        return true;
    }
    return false;
}