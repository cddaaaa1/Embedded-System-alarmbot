#include <Arduino.h>
#pragma once



extern int alarmFocus; 
extern int todoListCursor; // 在 ToDoList 界面操作时，用来标记光标位置

enum MainState {
    INIT,
    MAIN_MENU,
    ALARM_MENU,         // 进入报警二级菜单
    ALARM_SET_COUNTDOWN,// 新增：用于设置倒计时
    ALARM_SET_TIMER,    // 新增：用于设置定时闹钟
    TODO_MENU,
    COUNTDOWN,
    ALARM_TIMER_RUNNING,// 新增：定时闹钟启动后等待触发
    COUNTFINISH
};

// 声明全局变量
extern MainState currentState;
//position of cursor
extern int mainMenuIndex;
extern int alarmMenuIndex;
extern int todoMenuIndex;


extern const int leftPin;   // LEFT 按键引脚
extern const int rightPin;  // RIGHT 按键引脚
extern const int confirmPin;   // DOWN 按键引脚
extern const int returnPin;     // UP 按键引脚



// main manu items
enum MainMenuItems {
    MAIN_ALARM = 0,
    MAIN_TODO,
    MAIN_RETURN,
    MAIN_MENU_COUNT // 用于记录主菜单总数
};
// Alarm子菜单（第一级，选择要进哪个功能）
enum AlarmSubMenuItems {
    ALARM_SUB_COUNTDOWN = 0,
    ALARM_SUB_TIMER,
    ALARM_SUB_RETURN,
    ALARM_SUB_COUNT
};
// ========== 新增：闹钟“焦点”枚举 ==========
enum AlarmFocus {
    FOCUS_HOUR = 0,
    FOCUS_MINUTE,
    FOCUS_SECOND,
    FOCUS_ALARM_ONOFF, // 打开/关闭闹钟
    FOCUS_CONFIRM,     // 确认
    FOCUS_COUNT
};

// todo menu items
enum TodoMenuItems {
    TODO_RETURN = 0,
    TODO_MENU_COUNT
};

struct TodoItem {
    String text;   // 待办项的文字描述
    bool done;     // 是否已经划掉
};

// 一个全局的数组或向量来存储所有的待办项
extern TodoItem todoList[]; 
extern int todoCount; 

// =========== state machin functions ===========

// initialise state machine;
void initStateMachine ();
void initPins();


// get input and update state
void update();

// return current state;
MainState getState();

// get item index for each menu
int getMainMenuIndex();
int getAlarmFocus();
int getTodoMenuIndex();
int getAlarmMenuIndex();


// handle serial input(for test) change to joystic for later version
void handleInput (char c);

void handlePotChange();

bool updateToDoList();