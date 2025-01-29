#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_SSD1306.h>
#include "MENU/StateMachine.h"
#include "FACE/Face.h"

// ========== OLED 配置 ==========
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1       // 如果 OLED 没有接 RST，则用 -1
#define OLED_ADDRESS  0x3C     // 常见地址是 0x3C 或 0x3D


extern Face* face; //创建表情对象
extern Adafruit_SSD1306 display;  // 创建屏幕对象

//初始化屏幕
void initOled();
//初始化表情
void initFace();

void render(const MainState& state);

void renderFace();                   // INIT状态  表情
void renderMainMenu(int selectedIndex);    // 主菜单
void renderAlarmMenu(int selectedIndex); 

void renderAlarmSetScreen(int focus);  // 闹钟子菜单
void renderTodoMenu(int selectedIndex);    // ToDo子菜单

void renderCountdownScreen();
void renderTimerRunningScreen();
void angryFace();
//void rendeAlarmSetting();
void renderToDoListScreen();

#endif // DISPLAY_MANAGER_H