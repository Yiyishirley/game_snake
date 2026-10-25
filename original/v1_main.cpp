#include <graphics.h>
#include <time.h>
#include <Windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <iostream>
#include "main.h"
using namespace std;
#define PI acos(-1)
#define WIDTH 1080
#define HEIGHT 720

#define BLOCK_XY 30     //小方格边长
#define BLOCK_JJ 2      //小方格之间的间距
COLORREF SNACK_COLOR_MAX = COLORREF RGB(255,182,193);    // 新蛇头颜色
COLORREF SNACK_COLOR_MIN = COLORREF RGB(255,182,193);  // 新蛇尾颜色
COLORREF BK_COLOR = COLORREF RGB(255,20,147);          // 新游戏围墙颜色
float SNACK_XY_MAX = 0;         //蛇最粗值
float SNACK_XY_MIN = 10;        //蛇最细值
#define GAMETIME 120            //游戏速度
#define LIZINUM 30              //粒子数量
int CAMERA_X = 0;               //相机x坐标,0为屏幕中间
int CAMERA_Y = 0;               //相机y坐标,0为屏幕中间
#define SHAKE 120               //抖动效果的帧数
int WINDOW_SHAKE = 0;			//记录抖动了多少次
int score = 0;                  //游戏分数

const int BLOCK_WIDTH = WIDTH / (BLOCK_XY + BLOCK_JJ);  //小方格x轴数量
const int BLOCK_HEIGHT = HEIGHT / (BLOCK_XY + BLOCK_JJ); //小方格y轴数量

const int PYI_X = WIDTH - BLOCK_WIDTH * (BLOCK_XY + BLOCK_JJ);  //x轴上的偏移量，因为分辨率和方格经常不能整除
const int PYI_Y = HEIGHT - BLOCK_HEIGHT * (BLOCK_XY + BLOCK_JJ);//y轴上的偏移量，因为分辨率和方格经常不能整除
const float G = 0.001f;         //重力
const float MC = 0.9999f;       //摩擦力
float jd = 0;

struct snack {
    int dx = 1, dy = 0;  //方向
    int snack_map[BLOCK_WIDTH * BLOCK_HEIGHT] = { 2,1,0 };  //初始化蛇数组,数组第一个是头部
    int snack_len = 3;   //蛇的长度
} Snack;

struct lizi {
    float x = 0, y = 0;    //粒子位置
    int sjx[6] = { -7, -7, 7, -7, -7, 7 };
    float dx, dy;  //粒子速度
    float jd = 0;  //粒子角度
    float djd = 0; //粒子旋转的角速度

    int life = 100; //生命
    int d_life = 0; //当前生命

    COLORREF RGB;//粒子颜色
} Lizi[LIZINUM];

void initlizi(int x, int y) {   //x,y为要显示的位置
    for (int i = 0; i < LIZINUM; i++) {
        Lizi[i].x = rand() % BLOCK_XY + x;
        Lizi[i].y = rand() % BLOCK_XY + y;
        Lizi[i].dx = (float)i / LIZINUM * (rand() % 20 - 10) / 10.0f;
        Lizi[i].dy = (float)i / LIZINUM * (rand() % 20 - 10) / 10.0f;
        Lizi[i].jd = rand() % 360;
        Lizi[i].djd = (rand() % 200 - 100) / 100.0f;
        Lizi[i].RGB = RGB(rand() % 256, rand() % 256, rand() % 256);  // 随机颜色
    }
}

void drawlizi() {
    for (int i = 0; i < LIZINUM; i++) {
        Lizi[i].x += Lizi[i].dx;
        Lizi[i].y += Lizi[i].dy;
        Lizi[i].dy += G;
        Lizi[i].jd += Lizi[i].djd;

        // 使用自定义颜色绘制圆形粒子
        setfillcolor(Lizi[i].RGB);
        fillellipse(Lizi[i].x, Lizi[i].y, 3, 3);  // 绘制小圆形粒子
    }
}

struct food {
    int food_x, food_y;   //食物坐标
    int z = -30;          //食物帧动画起始
    int jz = 20;          //食物帧动画结束
    int zmp = -30;        //食物当前帧
    int p = 0;            //帧动画是否结束
} Food;

int maptox(int map) {     //map数组转x坐标
    return (map % BLOCK_WIDTH);
}
int maptoy(int map) {     //map数组转y坐标
    return (map / BLOCK_WIDTH);
}
int xytomap(int x, int y) { //xy坐标转map
    return (y * BLOCK_WIDTH + x);
}

void gamewindowshake() {
    if (WINDOW_SHAKE < SHAKE) {
        int x = rand() % 30 - 15;
        int y = rand() % 30 - 15;
        CAMERA_X = x;
        CAMERA_Y = y;
        WINDOW_SHAKE++;
    }
    else {
        CAMERA_X = 0;
        CAMERA_Y = 0;
    }
}

void gamedrawpixal(int x, int y, COLORREF COLOR, float xy) {
    int py_x = PYI_X / 2;
    int py_y = PYI_Y / 2;
    float xy2 = xy / 2;
    setfillcolor(COLOR); setlinecolor(COLOR);
    fillrectangle(x * BLOCK_XY + x * BLOCK_JJ + xy2 + py_x + CAMERA_X, y * BLOCK_XY + y * BLOCK_JJ + xy2 + py_y + CAMERA_Y,
        BLOCK_XY + x * BLOCK_XY + x * BLOCK_JJ - xy2 + py_x + CAMERA_X, BLOCK_XY + y * BLOCK_XY + y * BLOCK_JJ - xy2 + py_y + CAMERA_Y);
};

void drawsnack() {
    int rp = (GetRValue(SNACK_COLOR_MAX) - GetRValue(SNACK_COLOR_MIN)) / (Snack.snack_len - 1);
    int gp = (GetGValue(SNACK_COLOR_MAX) - GetGValue(SNACK_COLOR_MIN)) / (Snack.snack_len - 1);
    int bp = (GetBValue(SNACK_COLOR_MAX) - GetBValue(SNACK_COLOR_MIN)) / (Snack.snack_len - 1);

    float pp = (SNACK_XY_MAX - SNACK_XY_MIN) / (Snack.snack_len - 1);

    gamedrawpixal(maptox(Snack.snack_map[0]), maptoy(Snack.snack_map[0]),
        SNACK_COLOR_MAX, SNACK_XY_MAX);

    for (int i = 1; i < Snack.snack_len - 1; i++) {
        gamedrawpixal(maptox(Snack.snack_map[i]), maptoy(Snack.snack_map[i]),
            COLORREF RGB(GetRValue(SNACK_COLOR_MAX) - (rp * i), GetGValue(SNACK_COLOR_MAX) - (gp * i), GetBValue(SNACK_COLOR_MAX) - (bp * i)), SNACK_XY_MAX - (pp * i));
    }

    gamedrawpixal(maptox(Snack.snack_map[Snack.snack_len - 1]), maptoy(Snack.snack_map[Snack.snack_len - 1]),
        SNACK_COLOR_MIN, SNACK_XY_MIN);
}

void foodinit() {
LOOP:
    Food.food_x = rand() % BLOCK_WIDTH;
    Food.food_y = rand() % BLOCK_HEIGHT;
    for (int i = 0; i < Snack.snack_len; i++) {
        if ((maptox(Snack.snack_map[i]) == Food.food_x) && (maptoy(Snack.snack_map[i]) == Food.food_y))
            goto LOOP;
    }
    Food.p = 0;

}

void gameinit() {
    Snack.dx = 1;
    Snack.dy = 0;
    Snack.snack_map[0] = 2;
    Snack.snack_map[1] = 1;
    Snack.snack_map[2] = 0;
    Snack.snack_len = 3;
    score = 0;  // Reset score when game initializes
}

void gameover() {
    settextcolor(WHITE);
    settextstyle(90, 50, _T("Consolas"));
    RECT r = { 0, 0, WIDTH ,HEIGHT };
    while (1) {
        if (_kbhit()) {
            char ch = _getch();
            if (ch == 13) {
                gameinit();
                foodinit();
                break;
            }
        }
        cleardevice();
        drawbk();
        drawfood();
        drawsnack();
        gamewindowshake();
        WINDOW_SHAKE = 0;
        drawtext(_T("Game Over!!"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        FlushBatchDraw();
    }
}

void drawfood() {
    if (!Food.p) {
        if (Food.zmp > Food.jz) {
            Food.zmp = Food.z;
            Food.p = !Food.p;
        }
        float p = (Food.zmp * Food.zmp) / 10 - 100;
        gamedrawpixal(Food.food_x, Food.food_y, COLORREF RGB(255, 255, 255), p);
        //Sleep(100);
        Food.zmp++;
    }
    else {
        gamedrawpixal(Food.food_x, Food.food_y, COLORREF RGB(255, 255, 255), 0);
    }
}

void drawbk() {
    setfillcolor(BK_COLOR); setlinecolor(BK_COLOR);
    fillrectangle(0 + CAMERA_X, 0 + CAMERA_Y, PYI_X / 2 + CAMERA_X, HEIGHT + CAMERA_Y);
    fillrectangle((WIDTH - PYI_X / 2) + CAMERA_X, 0 + CAMERA_Y, WIDTH + CAMERA_X, HEIGHT + CAMERA_Y);
    fillrectangle(0 + CAMERA_X, 0 + CAMERA_Y, WIDTH + CAMERA_X, PYI_Y / 2 + CAMERA_Y);
    fillrectangle(0 + CAMERA_X, HEIGHT + CAMERA_Y, WIDTH + CAMERA_X, (HEIGHT - PYI_Y / 2) + CAMERA_Y);

    // 显示分数，设置文本背景为透明
    settextcolor(WHITE);
    setbkmode(TRANSPARENT);  // 设置文本背景为透明
    settextstyle(50,0, _T("Consolas")); // 设置字体大小为20，根据需要调整
    TCHAR scoreStr[100];
    _stprintf_s(scoreStr, _countof(scoreStr), _T("Score: %d"), score);
    outtextxy(10, 10, scoreStr);  // 在窗口左上角显示分数
}


void gamepause() {
    settextcolor(WHITE);
    settextstyle(90, 50, _T("Consolas"));
    RECT r = { 0, 0, WIDTH ,HEIGHT };
    drawtext(_T("Pause!"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    FlushBatchDraw();            //将缓冲输出到屏幕
    while (1) {
        if (_kbhit()) {
            char ch = _getch();
            if (ch == 32) {
                break;
            }
        }
        FlushBatchDraw();            //将缓冲输出到屏幕
    }
}

void gameupdata() {
    char ch;
    if (_kbhit()) {
        ch = _getch();   //up 119  down 115  left 97  right 100  \n 13  " " 32
        cout << "Key pressed: " << static_cast<int>(ch) << endl;
        switch (ch) {
        case 119: // 'w'
            if (!(Snack.dx == 0 && Snack.dy == 1)) {
                Snack.dx = 0;
                Snack.dy = -1;
                cout << "Moving UP" << endl;
            }
            break;
        case 115: // 's'
            if (!(Snack.dx == 0 && Snack.dy == -1)) {
                Snack.dx = 0;
                Snack.dy = 1;
                cout << "Moving DOWN" << endl;
            }
            break;
        case 97: // 'a'
            if (!(Snack.dx == 1 && Snack.dy == 0)) {
                Snack.dx = -1;
                Snack.dy = 0;
                cout << "Moving LEFT" << endl;
            }
            break;
        case 100: // 'd'
            if (!(Snack.dx == -1 && Snack.dy == 0)) {
                Snack.dx = 1;
                Snack.dy = 0;
                cout << "Moving RIGHT" << endl;
            }
            break;
        case 32: // Space bar
            gamepause(); //游戏暂停
            cout << "Game Paused" << endl;
            break;
        }
    }

    for (int i = 0; i < Snack.snack_len; i++) {
        Snack.snack_map[Snack.snack_len - i] = Snack.snack_map[Snack.snack_len - i - 1];
    }
    //墙壁碰撞检测
    if (maptox(Snack.snack_map[1]) + Snack.dx >= BLOCK_WIDTH || maptox(Snack.snack_map[1]) + Snack.dx < 0) {
        gameover();
    }
    if (maptoy(Snack.snack_map[1]) + Snack.dy >= BLOCK_HEIGHT || maptoy(Snack.snack_map[1]) + Snack.dy < 0) {
        gameover();
    }
    Snack.snack_map[0] = xytomap(maptox(Snack.snack_map[1]) + Snack.dx, maptoy(Snack.snack_map[1]) + Snack.dy);
    //食物碰撞检测
    if (maptox(Snack.snack_map[0]) == Food.food_x && maptoy(Snack.snack_map[0]) == Food.food_y) {
        WINDOW_SHAKE = 0;
        initlizi(Food.food_x * (BLOCK_XY + BLOCK_JJ), Food.food_y * (BLOCK_XY + BLOCK_JJ));
        foodinit();
        Snack.snack_len++;
        score += 10;  // 每吃到一个食物增加10分
    }
    //蛇体碰撞检测
    for (int i = 1; i < Snack.snack_len + 1; i++) {
        if (maptox(Snack.snack_map[0]) == maptox(Snack.snack_map[i])
            && maptoy(Snack.snack_map[0]) == maptoy(Snack.snack_map[i])) {
            gameover();
        }
    }

}

int main() {
    srand((unsigned)time(NULL));    //随机数种子
    initgraph(WIDTH, HEIGHT);       //创建绘图窗口
    BeginBatchDraw();               //开始批量绘图
    DWORD t1, t2;
    t1 = t2 = GetTickCount();       //获取电脑开机时间
    foodinit();

    while (1) {
        cleardevice();
        drawlizi();
        drawfood();         //画食物
        drawsnack();        //画蛇
        gamewindowshake();  //屏幕抖动

        if (t2 - t1 > GAMETIME) {        //使游戏非堵塞延时
            gameupdata();
            t1 = t2;

        }

        drawbk();            //画墙壁
        FlushBatchDraw();            //将缓冲输出到屏幕
        t2 = GetTickCount();
    }
}
