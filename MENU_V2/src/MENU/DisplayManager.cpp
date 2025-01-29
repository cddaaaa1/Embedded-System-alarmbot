#include <Adafruit_SSD1306.h>
#include "MENU/DisplayManager.h"
#include "FACE/Face.h"
#include <Arduino.h>
#include "MENU/ClockSetting.h"
#include "MENU/StateMachine.h"

// 定义全局对象
Face *face;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 初始化 OLED
void initOled(){
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;);
    }
    display.clearDisplay();
    display.display();
    display.setTextSize(1);
    display.setTextColor(WHITE);
}

void initFace(){
    // 创建Face对象
    face = new Face(128, 64, 40);
    face->Expression.GoTo_Normal();
    // 设置NORMAL表情
    face->Behavior.SetEmotion(Normal, 1.0);
    // 开启自动眨眼，并设定眨眼间隔
    face->RandomBlink = true;
    face->Blink.Timer.SetIntervalMillis(4000);

    // 关闭随机看向
    face->RandomLook = true;
    // face->Update();
}

void render(const MainState& state){
    
    // display.setTextSize(1);
    // display.setTextColor(WHITE);

    switch (state) {
    case INIT:
        renderFace();
        break;
    case MAIN_MENU:
        display.clearDisplay();
        renderMainMenu(getMainMenuIndex());
        display.display();
        break;
    case ALARM_MENU:
        display.clearDisplay();
        renderAlarmMenu(getAlarmMenuIndex());
        display.display();
        break;
    case TODO_MENU:
        display.clearDisplay();
        renderToDoListScreen();
        display.display();
        break;
    case ALARM_SET_COUNTDOWN:
        display.clearDisplay();
        renderAlarmSetScreen(getAlarmFocus());
        display.display();
        break;
    case ALARM_SET_TIMER:
        display.clearDisplay();
        renderAlarmSetScreen(getAlarmFocus());
        display.display();
        break;
    case ALARM_TIMER_RUNNING:
        display.clearDisplay();
        renderTimerRunningScreen();
        display.display();
        break;
    // ========== 新增的倒计时状态渲染 ========== 
    case COUNTDOWN:
        display.clearDisplay();
        renderCountdownScreen(); // 你自己实现的函数
        display.display();
        break;
    case COUNTFINISH:
        angryFace(); 
    default:
        // do nothing
        break;
    }
    // 因为face->update有clear和display（u82g）所以displayclear和display必须放进case里面
    
}

void renderFace(){
    face->Behavior.SetEmotion(Angry, 0);
    face->Behavior.SetEmotion(Normal, 1);
    face->Update();
}
// ========== 主菜单渲染 ==========
// 主菜单有 3 个选项： Alarm, ToDo, Return
void renderMainMenu(int selectedIndex) {
    display.setCursor(0, 0);
    display.println(F("=== Main Menu ==="));

    // 第一个选项: Alarm
    display.setCursor(0, 16);
    if (selectedIndex == MAIN_ALARM) display.print("-> ");
    else display.print("   ");
    display.println(F("Alarm"));

    // 第二个选项: ToDo
    display.setCursor(0, 26);
    if (selectedIndex == MAIN_TODO) display.print("-> ");
    else display.print("   ");
    display.println(F("To-Do"));

    // 第三个选项: Return
    display.setCursor(0, 36);
    if (selectedIndex == MAIN_RETURN) display.print("-> ");
    else display.print("   ");
    display.println(F("Return"));
}


// ========== 闹钟菜单渲染 ==========
// 这里只示例一个“Return”选项
void renderAlarmMenu(int selectedIndex) {
    display.setCursor(0, 0);
    display.println(F("=== Alarm Menu ==="));

    display.setCursor(0, 16);
    if (selectedIndex == ALARM_SUB_COUNTDOWN) display.print("-> ");
    else display.print("   ");
    display.println(F("counter"));

    // 第二个选项:
    display.setCursor(0, 26);
    if (selectedIndex == ALARM_SUB_TIMER) display.print("-> ");
    else display.print("   ");
    display.println(F("alarm"));

    // 第三个选项: Return
    display.setCursor(0, 36);
    if (selectedIndex == ALARM_SUB_RETURN) display.print("-> ");
    else display.print("   ");
    display.println(F("Return"));
}


// ========== ToDo 菜单渲染 ==========
// 这里也仅示例一个“Return”选项
void renderTodoMenu(int selectedIndex) {
    display.setCursor(0, 0);
    display.println(F("=== To-Do Menu ==="));

    display.setCursor(0, 16);
    if (selectedIndex == TODO_RETURN) display.print("-> ");
    else display.print("   ");
    display.println(F("Return"));
}


// ============ 闹钟编辑界面 ============
void renderAlarmSetScreen(int focus){
    display.clearDisplay();

    //示例：第一行显示日期（此处仅演示写死）
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("countdown_setting");

    // 第二行显示“HH:MM:SS”并根据 focus 高亮/加下划线等
    // 先把闹钟时间取出来
    int h = alarmTime.hour;
    int m = alarmTime.minute;
    int s = alarmTime.second;
    bool on = alarmOn;

    // 把它格式化成字符串
    //   如果想更大字体，可以 setTextSize(2) 甚至 setTextSize(3)
    display.setTextSize(2);
    display.setCursor(0, 16);

    // 为了演示“高亮”，我们可以在焦点的位置外围加方括号
    // 也可以简单地在focus处后面加个下划线之类
    // 这里做一个“[xx]”的做法
    
    // 小时
    if(focus == FOCUS_HOUR) {
      display.print("[");
      if(h<10) display.print("0");
      display.print(h);
      display.print("]");
    } else {
      if(h<10) display.print("0");
      display.print(h);
    }
    display.print(":");

    // 分钟
    if(focus == FOCUS_MINUTE) {
      display.print("[");
      if(m<10) display.print("0");
      display.print(m);
      display.print("]");
    } else {
      if(m<10) display.print("0");
      display.print(m);
    }
    display.print(":");

    // 秒
    if(focus == FOCUS_SECOND) {
      display.print("[");
      if(s<10) display.print("0");
      display.print(s);
      display.print("]");
    } else {
      if(s<10) display.print("0");
      display.print(s);
    }

    // 第三行显示 Alarm On/Off
    display.setTextSize(1);
    display.setCursor(0, 40);
    if(focus == FOCUS_ALARM_ONOFF) display.print(">");
    display.print("Alarm ");
    display.println(on ? "On" : "Off");

    // 第四行显示确认按钮
    // 如果焦点在确认上，就显示“> Confirm”，否则“Confirm”
    display.setCursor(0, 52);
    if(focus == FOCUS_CONFIRM) display.print("> Confirm(S)");
    else display.print("Confirm(S) Return(R)");

    display.display();
}



void renderCountdownScreen(){
    // countdownTime 在 ClockSet.cpp 里是全局
    // countdownStopped 也是全局
    int h = countdownTime.hour;
    int m = countdownTime.minute;
    int s = countdownTime.second;

    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("COUNTDOWN MODE");

    // 显示当前倒计时时间
    display.setTextSize(2);
    display.setCursor(0,16);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
    display.print(buf);

    // 显示暂停/继续状态
    display.setTextSize(1);
    display.setCursor(0, 40);
    if(countdownStopped){
        display.print("Stopped...");
    } else {
        display.print("Running...");
    }

    // 显示操作提示
    display.setCursor(0, 52);
    display.print("S=Stop/Resume  R=Reset");

    display.display();
}


void renderTimerRunningScreen() {
    // 假设 `currentHour`, `currentMinute`, `currentSecond` 是全局时间变量
    extern int currentHour;
    extern int currentMinute;
    extern int currentSecond;

    // 假设 `timedAlarmTime` 是全局变量
    extern AlarmTime timedAlarmTime;

    // 显示标题
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("TIMER RUNNING");

    // 显示系统当前时间
    display.setTextSize(1);
    display.setCursor(0, 10);
    char currentTimeBuf[16];
    snprintf(currentTimeBuf, sizeof(currentTimeBuf), "Now: %02d:%02d:%02d",
             currentHour, currentMinute, currentSecond);
    display.println(currentTimeBuf);

    // 显示定时闹钟时间
    display.setCursor(0, 20);
    char alarmTimeBuf[16];
    snprintf(alarmTimeBuf, sizeof(alarmTimeBuf), "Alarm: %02d:%02d:%02d",
             timedAlarmTime.hour, timedAlarmTime.minute, timedAlarmTime.second);
    display.println(alarmTimeBuf);

    // 显示闹钟状态（On/Off）
    display.setCursor(0, 30);
    if (alarmOn) {
        display.println("Alarm Status: On");
    } else {
        display.println("Alarm Status: Off");
    }

    // 显示操作提示
    display.setCursor(0, 50);
    display.print("X=Return");

    // 更新屏幕
    display.display();
}

void angryFace(){
    face->Behavior.SetEmotion(Normal, 0);
    face->Behavior.SetEmotion(Angry, 1.0);
    face->Update();
}

void renderToDoListScreen() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("=== ToDo List ==="));

    // 每一行显示一个待办项
    const int lineHeight = 10; // 根据你的oled字体大小
    int startY = 16;           // 开始显示的Y位置

    for(int i = 0; i < todoCount; i++){
        int y = startY + i * lineHeight;

        // 光标跳到相应位置
        display.setCursor(0, y);

        // 如果是当前光标所在项，就显示 "-> "
        if (i == todoListCursor) {
            display.print("-> ");
        } else {
            display.print("   ");
        }

        // 如果该项被done了（划掉），可以加个标记
        if (todoList[i].done) {
            display.print("[X] ");
        } else {
            display.print("[ ] ");
        }

        // 显示文本
        display.println(todoList[i].text);
    }

    display.display();
}