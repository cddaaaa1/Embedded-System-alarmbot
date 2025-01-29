#ifndef CLOCK_SET_H
#define CLOCK_SET_H

extern const int potentiometerPin;  // 电位器接到模拟引脚 A0
extern int lastPotValue;            // 上一次电位器值
extern int currentPotValue;         // 当前电位器值
extern int threshold;              // 变化阈值，避免过于灵敏



// 用于记录闹钟的时、分、秒
typedef struct {
    int hour;
    int minute;
    int second;
} AlarmTime;

extern AlarmTime alarmTime;

extern bool alarmOn;  // 新增：闹钟是否开启

// 初始化闹钟为默认值，比如 0:0:0
void initAlarmTime();

// 下面这几个函数用于调节时、分、秒，超出范围时自动回绕
void increaseHour();
void decreaseHour();
void increaseMinute();
void decreaseMinute();
void increaseSecond();
void decreaseSecond();
// 打开/关闭闹钟
void toggleAlarmOnOff();


// ========== 新增倒计时支持 ==========
// countdownInitialTime：存储开始时的“初始倒计时”
// countdownTime：当前剩余倒计时
// countdownStopped：是否处于暂停状态

extern AlarmTime countdownInitialTime;
extern AlarmTime countdownTime;
extern bool countdownStopped;

// 在确认闹钟时，初始化倒计时(把 alarmTime 的值拷贝给 countdownInitialTime 和 countdownTime)
void initCountdown();
void resetCountdown();           // 把 countdownTime 重置回 countdownInitialTime
bool countdownTickOneSecond();   // 每秒调用一次；减少1秒；如果到0返回true
bool isCountdownZero();          // 检查倒计时是否 0



// 闹钟(定时)相关
extern AlarmTime timedAlarmTime;     // 用来存储“定时闹钟”目标
extern bool timedAlarmTriggered;
void initAlarm();       // 把 alarmTime 复制到 timedAlarmTime
bool checkAlarm();      // 每秒调用，检查是否到达 timedAlarmTime
#endif
