#include "MENU/StateMachine.h"
#include <Arduino.h> //for serial display
#include "MENU/ClockSetting.h"  // 用于闹钟时分秒的增减
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

// 定义全局变量
MainState currentState = INIT;
int mainMenuIndex = 0;
int alarmMenuIndex = 0;
int todoMenuIndex = 0;

int todoListCursor = 0;
TodoItem todoList[10];  // 假设最多存10条
int todoCount = 0;

const int confirmPin = 25;     // UP 按键引脚
const int leftPin =26;   // LEFT 按键引脚
const int returnPin = 14;   // DOWN 按键引脚
const int rightPin = 27;  // RIGHT 按键引脚

// 记录上一次“稳定状态”的布尔值 (HIGH=未按下, LOW=按下)
bool upState = HIGH;     
bool confirmState = HIGH;
bool downState = HIGH;
bool ReturnState = HIGH;

// 记录最近一次检测到状态切换的时间(毫秒)
unsigned long confirmLastChangeTime = 0;
unsigned long leftLastChangeTime = 0;
unsigned long ReturnLastChangeTime = 0;
unsigned long rightLastChangeTime = 0;

// 防抖时长，单位毫秒，根据需求可调
const unsigned long DEBOUNCE_MS = 50;
//闹钟焦点
int alarmFocus = FOCUS_HOUR;
void initPins(){
    pinMode(confirmPin, INPUT_PULLUP);     // 设置UP按键为输入模式，启用内部上拉
    pinMode(leftPin, INPUT_PULLUP);   // 设置LEFT按键为输入模式，启用内部上拉
    pinMode(returnPin, INPUT_PULLUP);   // 设置DOWN按键为输入模式，启用内部上拉
    pinMode(rightPin, INPUT_PULLUP);  // 设置RIGHT按键为输入模式，启用内部上拉
    
}
// initialise state machine;
void initStateMachine(){
    currentState = INIT;
    mainMenuIndex = 0;
    // alarmMenuIndex = 0;
    todoMenuIndex = 0;
    alarmFocus    = FOCUS_HOUR;
    initAlarmTime(); // 初始化闹钟(时分秒=0)和对应的potentialmeter
    initPins();
    Serial.println("State machine init done.");
}
/** 
 * 这里假设我们用 millis() 或者定时器，每隔 1000ms 就调用一次 update() 
 * 以便让 countdownTickOneSecond() 减少 1秒
 */
static unsigned long lastUpdateTime = 0;

// 每次循环都要调用 update()，做按键去抖 & 倒计时逻辑
void update() {
    // ------------- 1) 防抖 + 读取“确认”按键 -------------
    bool rawUpReading = digitalRead(confirmPin);
    if (rawUpReading != upState) {
        if (millis() - confirmLastChangeTime > DEBOUNCE_MS) {
            upState = rawUpReading;
            if (upState == LOW) {
                // 按下时
                handleInput('C');  // 'C'代表“Confirm”
            }
        }
    } else {
        confirmLastChangeTime = millis();
    }

    // ------------- 2) 防抖 + 读取“左”按键 -------------
    bool rawLeftReading = digitalRead(leftPin);
    if (rawLeftReading != confirmState) {
        if (millis() - leftLastChangeTime > DEBOUNCE_MS) {
            confirmState = rawLeftReading;
            if (confirmState == LOW) {
                handleInput('L');  // 'L'代表“Left”
            }
        }
    } else {
        leftLastChangeTime = millis();
    }

    // ------------- 3) 防抖 + 读取“返回”按键 -------------
    bool rawDownReading = digitalRead(returnPin);
    if (rawDownReading != downState) {
        if (millis() - ReturnLastChangeTime > DEBOUNCE_MS) {
            downState = rawDownReading;
            if (downState == LOW) {
                handleInput('X'); // 'X'代表“Return”
            }
        }
    } else {
        ReturnLastChangeTime = millis();
    }

    // ------------- 4) 防抖 + 读取“右”按键 -------------
    bool rawRightReading = digitalRead(rightPin);
    if (rawRightReading != ReturnState) {
        if (millis() - rightLastChangeTime > DEBOUNCE_MS) {
            ReturnState = rawRightReading;
            if (ReturnState == LOW) {
                handleInput('R');  // 'R'代表“Right”
            }
        }
    } else {
        rightLastChangeTime = millis();
    }

    // ------------- 5) 如果在 ALARM_MENU，就让电位器来调节时分秒 -------------
    if (currentState == ALARM_SET_COUNTDOWN||currentState == ALARM_SET_TIMER) {
        handlePotChange(); 
    }

    // ------------- 6) 如果在 COUNTDOWN，就执行倒计时逻辑 -------------
    if (currentState == COUNTDOWN) {
        unsigned long now = millis();
        if (now - lastUpdateTime >= 1000) {
            lastUpdateTime = now;
            bool reachZero = countdownTickOneSecond(); // 你在ClockSetting.cpp里实现
            if (reachZero) {
                Serial.println("[COUNTDOWN] Time is up!");
                currentState = COUNTFINISH;
            }
        }
    }
    // 4) 如果在 ALARM_TIMER_RUNNING 或者无论在哪个状态，都可检查闹钟
    if(currentState == ALARM_TIMER_RUNNING) {
        if(checkAlarm()) {
            // 触发闹钟
            // 例如播放蜂鸣器、显示闪烁，然后返回主菜单
            currentState = COUNTFINISH;
            Serial.println(">>> ALARM RING !!! <<<");
        }
    }
}

// 处理输入事件，进行状态切换 & 业务逻辑
void handleInput(char c){
    switch (currentState) {

    case INIT: {
        if(c == 'C'){ 
            // 按“确认”进入主菜单
            currentState = MAIN_MENU;
            Serial.println("[INIT] confirm -> Enter MAIN_MENU");
        }
        break;
    }

    case MAIN_MENU: {
        switch(c){
            case 'L': 
                // 光标上移
                mainMenuIndex--;
                if (mainMenuIndex < 0) mainMenuIndex = MAIN_MENU_COUNT - 1;
                break;
            case 'R': 
                // 光标下移
                mainMenuIndex++;
                if (mainMenuIndex >= MAIN_MENU_COUNT) mainMenuIndex = 0;
                break;
            case 'C': 
                // 选中当前菜单项
                switch(mainMenuIndex){
                    case MAIN_ALARM:
                        currentState = ALARM_MENU;
                        Serial.println("[MAIN_MENU] -> ALARM_MENU");
                        break;
                    case MAIN_TODO:
                            // ============ 新增逻辑：读取JSON并显示 ToDo ============
                        if (updateToDoList()) {
                            // 读取成功
                            todoListCursor = 0; // 光标重置到第 0 项
                            currentState = TODO_MENU;
                            Serial.println("[MAIN_MENU] -> TODO_MENU");
                        } else {
                            // 如果读取失败，根据需求给出提示
                            Serial.println("[MAIN_MENU] updateToDoList FAILED!");
                            // 依旧可以进入 TODO_MENU，或者直接返回
                            currentState = TODO_MENU;
                        }
                        break;
                    case MAIN_RETURN:
                        // 返回到INIT
                        currentState = INIT;
                        Serial.println("[MAIN_MENU] -> INIT");
                        break;
                }
                break;
        }
        break;
    }
    // =========== 新增：Alarm 二级菜单 ===========
    case ALARM_MENU:
        switch(c) {
            // L => 光标上
            case 'L':
                alarmMenuIndex--;
                if(alarmMenuIndex < 0) alarmMenuIndex = ALARM_SUB_COUNT - 1;
                break;
            // R => 光标下
            case 'R':
                alarmMenuIndex++;
                if(alarmMenuIndex >= ALARM_SUB_COUNT) alarmMenuIndex = 0;
                break;
            // C => 确认
            case 'C':
                switch(alarmMenuIndex) {
                    case ALARM_SUB_COUNTDOWN:
                        // 进入“设置倒计时”状态
                        currentState = ALARM_SET_COUNTDOWN;
                        // 在 ALARM_SET_COUNTDOWN 里再使用 alarmFocus=FOCUS_HOUR 等
                        Serial.println("[ALARM_MENU] => ALARM_SET_COUNTDOWN");
                        break;

                    case ALARM_SUB_TIMER:
                        // 进入“设置定时闹钟”状态
                        currentState = ALARM_SET_TIMER;
                        Serial.println("[ALARM_MENU] => ALARM_SET_TIMER");
                        break;

                    case ALARM_SUB_RETURN:
                        // 返回主菜单
                        currentState = MAIN_MENU;
                        Serial.println("[ALARM_MENU] => MAIN_MENU (Return)");
                        break;
                }
                break;
            // 如果想加一个“X”返回也可以
            case 'X':
                currentState = MAIN_MENU;
                Serial.println("[ALARM_MENU] => MAIN_MENU (Return)");
                break;
        }
        break;

    case ALARM_SET_COUNTDOWN: {
        switch(c){
            // 左右切换焦点
            case 'L':
                alarmFocus--;
                if (alarmFocus < 0) alarmFocus = FOCUS_COUNT - 1;
                break;
            case 'R':
                alarmFocus++;
                if (alarmFocus >= FOCUS_COUNT) alarmFocus = 0;
                break;

            // X => 返回主菜单
            case 'X':
                currentState = MAIN_MENU;
                Serial.println("[ALARM_MENU] -> MAIN_MENU (Return)");
                break;

            // S => 确认 => 进入 COUNTDOWN
            case 'C':
                if(alarmFocus == FOCUS_CONFIRM){
                    initCountdown(); // 复制 alarmTime => countdownTime
                    currentState = COUNTDOWN;
                    Serial.println("[ALARM_MENU] -> COUNTDOWN");
                    Serial.printf("Time = %02d:%02d:%02d, AlarmOn=%s\n",
                        alarmTime.hour, alarmTime.minute, alarmTime.second,
                        alarmOn ? "true" : "false"
                    );
                }
                break;
        }
        break;
    }
    // =========== 新增：定时闹钟设置状态 ===========
    case ALARM_SET_TIMER:
        switch(c) {
            case 'L':
                alarmFocus--;
                if(alarmFocus < 0) alarmFocus = FOCUS_COUNT-1;
                break;
            case 'R':
                alarmFocus++;
                if(alarmFocus >= FOCUS_COUNT) alarmFocus = 0;
                break;
            case 'X':
                currentState = ALARM_MENU;
                Serial.println("[ALARM_SET_TIMER] => ALARM_MENU (Return)");
                break;
            case 'C':
                if(alarmFocus == FOCUS_CONFIRM) {
                    // =========== 关键：初始化定时闹钟 ===========
                    initAlarm();  
                    currentState = ALARM_TIMER_RUNNING;
                    Serial.println("[ALARM_SET_TIMER] => ALARM_TIMER_RUNNING");
                }
                break;
        }
        break;
    case TODO_MENU: {
        switch(c){
            case 'L': 
                // 上移光标
                todoListCursor--;
                if (todoListCursor < 0) {
                    todoListCursor = todoCount - 1;  // 循环到最后一条
                }
                break;

            case 'R':
                // 下移光标
                todoListCursor++;
                if (todoListCursor >= todoCount) {
                    todoListCursor = 0; // 回到第 0 条
                }
                break;

            case 'C':
                // 划掉 <-> 复原
                if (todoListCursor >= 0 && todoListCursor < todoCount) {
                    // 取反
                    todoList[todoListCursor].done = !todoList[todoListCursor].done;
                }
                break;

            case 'X':
                // 返回主菜单
                currentState = MAIN_MENU;
                Serial.println("[TODO_MENU] -> MAIN_MENU");
                break;
        }
        break;
    }
    case COUNTDOWN: {
        switch(c){
            // L => Stop/Resume
            case 'L':
                countdownStopped = !countdownStopped;
                Serial.println(countdownStopped ? "[COUNTDOWN] => Stopped"
                                                : "[COUNTDOWN] => Resumed");
                break;
            // R => Reset
            case 'R':
                resetCountdown();
                Serial.println("[COUNTDOWN] => Reset to initial");
                break;
            // X => Return to MAIN_MENU
            case 'X':
                currentState = MAIN_MENU;
                Serial.println("[COUNTDOWN] => Return to MAIN_MENU");
                break;
        }
        break;
    }

    case ALARM_TIMER_RUNNING:
        switch(c){
            // 若需要在此状态按键返回主菜单，也可在此处理
            case 'X':
                // 用户放弃或返回
                currentState = MAIN_MENU;
                Serial.println("[ALARM_TIMER_RUNNING] => MAIN_MENU");
                break;
        }
        break;

    case COUNTFINISH: {
        // 如果想在“倒计时结束”状态下按某键回主菜单，可以写：
        if(c == 'C'){
            currentState = MAIN_MENU;
            Serial.println("[COUNTFINISH] => Return to MAIN_MENU");
        }
        break;
    }

    } // end switch(currentState)
}
MainState getState(){
    return currentState;
}

int getMainMenuIndex(){
    return mainMenuIndex;
}

int getAlarmMenuIndex(){
    return alarmMenuIndex;
}

int getAlarmFocus(){
    return alarmFocus;
}

int getTodoMenuIndex(){
    return todoMenuIndex;
}
void handlePotChange() {

    int potVal = analogRead(potentiometerPin); // 0 ~ 1023

    switch (alarmFocus)
    {
    case FOCUS_HOUR: {
        // 把0-1023映射到0-24；如果得到24就手动修正成23
        int newHour = map(potVal, 0, 4095, 0, 24);
        if (newHour == 24) newHour = 23;

        // 只有在数值变化时，才更新 alarmTime & 输出日志
        if (newHour != alarmTime.hour) {
            alarmTime.hour = newHour;
            Serial.printf("Hour => %d\n", newHour);
        }
        break;
    }
    case FOCUS_MINUTE:
    case FOCUS_SECOND: {
        // 分钟/秒都是0-59区间
        int newVal = map(potVal, 0, 4094, 0, 60);
        if (newVal == 60) newVal = 59;

        if (alarmFocus == FOCUS_MINUTE) {
            if (newVal != alarmTime.minute) {
                alarmTime.minute = newVal;
                Serial.printf("Minute => %d\n", newVal);
            }
        } else {
            // SECOND
            if (newVal != alarmTime.second) {
                alarmTime.second = newVal;
                Serial.printf("Second => %d\n", newVal);
            }
        }
        break;
    }
    case FOCUS_ALARM_ONOFF: {
        // 如果你也想用电位器控制“On/Off”，可以做个阈值：小于512=Off，大于512=On
        int midpoint = 512;
        bool newOn = (potVal > midpoint); 
        if (newOn != alarmOn) {
            alarmOn = newOn;
            Serial.print("AlarmOn => ");
            Serial.println(alarmOn ? "true" : "false");
        }
        break;
    }
    case FOCUS_CONFIRM:
        // 如果确认项也想通过电位器？一般不会这么用，但你可以自定义
        break;
    }
}
// 全局变量（需要在 .h 里声明 extern）

bool updateToDoList() {
    // 1) 先确保文件系统已Mount，例如 SPIFFS.begin()
    //    如果你用的是 SD.begin() 也类似
    if (!SPIFFS.begin(true)) {
        Serial.println("[updateToDoList] SPIFFS init failed!");
        return false;
    }

    // 2) 打开文件
    File file = SPIFFS.open("/todo.json", "r");
    if (!file) {
        Serial.println("[updateToDoList] Failed to open /todo.json");
        return false;
    }

    // 3) 解析 JSON
    StaticJsonDocument<1024> doc; 
    DeserializationError err = deserializeJson(doc, file);
    if (err) {
        Serial.print("[updateToDoList] deserializeJson() failed: ");
        Serial.println(err.f_str());
        file.close();
        return false;
    }

    // 4) 读取 "todos" 数组
    JsonArray arr = doc["todos"].as<JsonArray>();
    if (arr.isNull()) {
        Serial.println("[updateToDoList] 'todos' array not found in JSON!");
        file.close();
        return false;
    }

    // 5) 遍历数组，填充到 todoList
    int i = 0;
    for (JsonObject obj : arr) {
        if (i >= 10) break; // 最多10条
        todoList[i].text = obj["text"] | "";
        todoList[i].done = obj["done"] | false;
        i++;
    }
    todoCount = i;

    file.close();
    Serial.print("[updateToDoList] Loaded ToDo items = ");
    Serial.println(todoCount);
    return true;
}