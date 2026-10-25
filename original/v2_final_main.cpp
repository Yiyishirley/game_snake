#include <graphics.h>   // 包含绘图库的头文件
#include <time.h>       // 包含时间相关函数的头文件
#include <Windows.h>    // 包含 Windows API 函数的头文件
#include <math.h>       // 包含数学函数的头文件
#include <stdio.h>      // 包含标准输入输出函数的头文件
#include <stdlib.h>     // 包含标准库函数的头文件
#include <conio.h>      // 包含控制台输入输出函数的头文件
#include <iostream>     // 包含 C++ 标准输入输出流的头文件
#include "main.h"       // 包含自定义头文件 "main.h"

using namespace std;    // 使用标准命名空间

#define PI acos(-1)     // 定义宏 PI 为圆周率
#define WIDTH 1080      // 定义窗口宽度
#define HEIGHT 720      // 定义窗口高度

#define BLOCK_XY 30     // 定义小方格边长
#define BLOCK_JJ 2      // 定义小方格之间的间距
COLORREF SNACK_COLOR_MAX = COLORREF RGB(255, 182, 193);   // 新蛇头颜色
COLORREF SNACK_COLOR_MIN = COLORREF RGB(255, 182, 193);   // 新蛇尾颜色
COLORREF BK_COLOR = COLORREF RGB(255, 20, 147);           // 新游戏围墙颜色
float SNACK_XY_MAX = 0;        // 蛇最粗值
float SNACK_XY_MIN = 10;       // 蛇最细值
#define GAMETIME 100           // 游戏速度  
#define LIZINUM 30              // 粒子数量
int CAMERA_X = 0;               // 相机x坐标,0为屏幕中间
int CAMERA_Y = 0;               // 相机y坐标,0为屏幕中间
#define SHAKE 120               // 抖动效果的帧数
int WINDOW_SHAKE = 0;           // 记录抖动了多少次
int score = 0;                  // 游戏分数
int highestScore = 0;  // 用于存储最高分




const int BLOCK_WIDTH = WIDTH / (BLOCK_XY + BLOCK_JJ);   // 小方格x轴数量
const int BLOCK_HEIGHT = HEIGHT / (BLOCK_XY + BLOCK_JJ); // 小方格y轴数量

const int PYI_X = WIDTH - BLOCK_WIDTH * (BLOCK_XY + BLOCK_JJ); // x轴上的偏移量，因为分辨率和方格经常不能整除
const int PYI_Y = HEIGHT - BLOCK_HEIGHT * (BLOCK_XY + BLOCK_JJ);// y轴上的偏移量，因为分辨率和方格经常不能整除
const float G = 0.001f;         // 重力
const float MC = 0.9999f;       // 摩擦力
float jd = 0;                   // 角度变量

struct snack {                  // 定义蛇结构体
    int dx = 1, dy = 0;         // 方向
    int snack_map[BLOCK_WIDTH * BLOCK_HEIGHT] = { 2,1,0 };  // 初始化蛇数组,数组第一个是头部
    int snack_len = 3;          // 蛇的长度
} Snack;

struct lizi {                   // 定义粒子结构体
    float x = 0, y = 0;         // 粒子位置
    int sjx[6] = { -7, -7, 7, -7, -7, 7 };  // 粒子形状
    float dx, dy;               // 粒子速度
    float jd = 0;               // 粒子角度
    float djd = 0;              // 粒子旋转的角速度
    int life = 100;             // 生命
    int d_life = 0;             // 当前生命
    COLORREF RGB;               // 粒子颜色
} Lizi[LIZINUM];

#define M_PI 3.14159265358979323846  // 定义宏 PI




void initlizi(int x, int y) {  // 初始化粒子函数，传入参数 x 和 y
    for (int i = 0; i < LIZINUM; i++) {  // 循环遍历每个粒子
        Lizi[i].x = rand() % BLOCK_XY + x;  // 为粒子的 x 坐标赋值，加上了偏移量 x
        Lizi[i].y = rand() % BLOCK_XY + y;  // 为粒子的 y 坐标赋值，加上了偏移量 y
        float angle = 2 * M_PI * rand() / RAND_MAX;  // 产生0到2π之间的随机角度
        float speed = 0.5 * sqrt(rand() / (float)RAND_MAX);  // 速度根据需要进行调整
        Lizi[i].dx = speed * cos(angle);  // 粒子 x 方向速度
        Lizi[i].dy = speed * sin(angle);  // 粒子 y 方向速度
        Lizi[i].jd = rand() % 36000;  // 粒子旋转角度
        Lizi[i].djd = (rand() % 200 - 100) / 100.0f; // 粒子旋转的角速度
        Lizi[i].RGB = RGB(rand() % 256, rand() % 256, rand() % 256); // 随机生成粒子颜色
    }
}


void drawlizi() {  // 绘制粒子函数
    for (int i = 0; i < LIZINUM; i++) {  // 循环遍历每个粒子
        Lizi[i].x += Lizi[i].dx;  // 更新粒子 x 坐标
        Lizi[i].y += Lizi[i].dy;  // 更新粒子 y 坐标
        Lizi[i].dy += G;          // 更新粒子 y 方向速度，加上重力
        Lizi[i].jd += Lizi[i].djd; // 更新粒子旋转角度

        // 根据粒子的旋转角度和形状绘制不同样式的粒子
        setfillcolor(Lizi[i].RGB);  // 设置填充颜色
        float angle = Lizi[i].jd * M_PI / 180.0f;  // 将角度转换为弧度
        int radius = 5;  // 粒子的半径

        // 计算粒子的绘制位置
        int px = Lizi[i].x + radius * cos(angle);  // 计算粒子 x 坐标
        int py = Lizi[i].y + radius * sin(angle);  // 计算粒子 y 坐标

        // 绘制粒子，这里假设使用矩形作为粒子的形状
        fillrectangle(px - radius, py - radius, px + radius, py + radius);  // 绘制矩形粒子
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

void gamewindowshake() {  // 定义屏幕抖动函数
    if (WINDOW_SHAKE < SHAKE) {  // 如果抖动次数小于设定的抖动次数
        int x = rand() % 30 - 15;  // 生成随机 x 坐标偏移量，范围为[-15, 15]
        int y = rand() % 30 - 15;  // 生成随机 y 坐标偏移量，范围为[-15, 15]
        CAMERA_X = x;  // 更新相机 x 坐标
        CAMERA_Y = y;  // 更新相机 y 坐标
        WINDOW_SHAKE++;  // 抖动次数加一
    }
    else {  // 如果抖动次数已达到设定的抖动次数
        CAMERA_X = 0;  // 将相机 x 坐标重置为0
        CAMERA_Y = 0;  // 将相机 y 坐标重置为0
    }
}

void gamedrawpixal(int x, int y, COLORREF COLOR, float xy) {  // 定义绘制像素函数，传入参数为 x 坐标、y 坐标、颜色、像素大小
    int py_x = PYI_X / 2;  // 计算 x 轴偏移量
    int py_y = PYI_Y / 2;  // 计算 y 轴偏移量
    float xy2 = xy / 2;    // 计算像素大小的一半
    setfillcolor(COLOR);   // 设置填充颜色
    setlinecolor(COLOR);   // 设置线条颜色
    fillrectangle(x * BLOCK_XY + x * BLOCK_JJ + xy2 + py_x + CAMERA_X, y * BLOCK_XY + y * BLOCK_JJ + xy2 + py_y + CAMERA_Y,  // 绘制矩形，左上角坐标
        BLOCK_XY + x * BLOCK_XY + x * BLOCK_JJ - xy2 + py_x + CAMERA_X, BLOCK_XY + y * BLOCK_XY + y * BLOCK_JJ - xy2 + py_y + CAMERA_Y);  // 绘制矩形，右下角坐标
}

void drawsnack() {  // 定义绘制蛇的函数
    int rp = (GetRValue(SNACK_COLOR_MAX) - GetRValue(SNACK_COLOR_MIN)) / (Snack.snack_len - 1);  // 计算红色通道每个格子的颜色增量
    int gp = (GetGValue(SNACK_COLOR_MAX) - GetGValue(SNACK_COLOR_MIN)) / (Snack.snack_len - 1);  // 计算绿色通道每个格子的颜色增量
    int bp = (GetBValue(SNACK_COLOR_MAX) - GetBValue(SNACK_COLOR_MIN)) / (Snack.snack_len - 1);  // 计算蓝色通道每个格子的颜色增量
    float pp = (SNACK_XY_MAX - SNACK_XY_MIN) / (Snack.snack_len - 1);  // 计算每个格子的像素大小增量

    // 绘制蛇头
    gamedrawpixal(maptox(Snack.snack_map[0]), maptoy(Snack.snack_map[0]), SNACK_COLOR_MAX, SNACK_XY_MAX);

    // 绘制蛇身
    for (int i = 1; i < Snack.snack_len - 1; i++) {
        gamedrawpixal(maptox(Snack.snack_map[i]), maptoy(Snack.snack_map[i]),
            COLORREF RGB(GetRValue(SNACK_COLOR_MAX) - (rp * i), GetGValue(SNACK_COLOR_MAX) - (gp * i), GetBValue(SNACK_COLOR_MAX) - (bp * i)), SNACK_XY_MAX - (pp * i));
    }

    // 绘制蛇尾
    gamedrawpixal(maptox(Snack.snack_map[Snack.snack_len - 1]), maptoy(Snack.snack_map[Snack.snack_len - 1]), SNACK_COLOR_MIN, SNACK_XY_MIN);
}





void foodinit() {  // 定义食物初始化函数
LOOP:  // 定义标签LOOP，用于循环
    Food.food_x = rand() % BLOCK_WIDTH;  // 随机生成食物的 x 坐标
    Food.food_y = rand() % BLOCK_HEIGHT;  // 随机生成食物的 y 坐标
    for (int i = 0; i < Snack.snack_len; i++) {  // 遍历蛇的身体
        if ((maptox(Snack.snack_map[i]) == Food.food_x) && (maptoy(Snack.snack_map[i]) == Food.food_y))  // 如果食物坐标与蛇的身体重叠
            goto LOOP;  // 跳转到标签LOOP，重新生成食物坐标
    }
    Food.p = 0;  // 将食物帧动画状态重置为0
}




void gameinit() {
    PlaySound(NULL, NULL, 0);  // 停止当前播放的音乐
    startBackgroundMusic();    // 重新开始播放背景音乐
    Snack.dx = 1;
    Snack.dy = 0;
    Snack.snack_map[0] = 2;
    Snack.snack_map[1] = 1;
    Snack.snack_map[2] = 0;
    Snack.snack_len = 3;
    score = 0;
    foodinit();                // 初始化食物
}

void gameover() {
    if (score > highestScore) {
        highestScore = score;  // 更新最高分
        // 可选：添加代码来保存最高分到文件，以便在游戏重新启动时载入
    }
    PlaySound(NULL, NULL, 0);  // 停止当前播放的音乐
    PlaySound(TEXT("D:\\output.wav"), NULL, SND_FILENAME | SND_ASYNC);  // 播放游戏结束音效

    settextcolor(WHITE);
    settextstyle(90, 50, _T("Consolas"));
    RECT r = { 0, 0, WIDTH, HEIGHT };

    while (1) {
        if (_kbhit()) {
            char ch = _getch();
            if (ch == 13) {  // 按下回车键重新开始游戏
                gameinit();  // 初始化游戏
                break;
            }
        }

        cleardevice();
        drawbk();
        drawfood();
        drawsnack();
        gamewindowshake();
        drawtext(_T("Game Over (-3-) ~ Press Enter to Restart"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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
    settextstyle(50, 0, _T("Consolas")); // 设置字体大小为20
    TCHAR scoreStr[100];
    _stprintf_s(scoreStr, _countof(scoreStr), _T("Score: %d"), score);
    outtextxy(10, 10, scoreStr);  // 在窗口左上角显示分数
    TCHAR highScoreStr[100]; 
    _stprintf_s(highScoreStr, _countof(highScoreStr), _T("High Score: %d"), highestScore); 
    outtextxy(10, 60, highScoreStr); 
}


void gamepause() {
    PlaySound(NULL, NULL, 0);  // 停止音乐播放
    settextcolor(WHITE);
    settextstyle(90, 50, _T("Consolas"));
    RECT r = { 0, 0, WIDTH, HEIGHT };
    drawtext(_T("Pause! ^3^ "), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    FlushBatchDraw();  // 将缓冲区内容输出到屏幕，显示暂停信息
    bool paused = true;
    while (paused) {
        if (_kbhit()) {
            char ch = _getch();
            if (ch == 32) {  // 空格键被按下，继续游戏
                paused = false;
            }
        }
        FlushBatchDraw();  // 持续刷新屏幕，以保持显示的暂停信息
    }

    startBackgroundMusic();  // 重新开始播放背景音乐
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

void startBackgroundMusic() {
    // 使用 SND_LOOP 和 SND_ASYNC 以循环和异步的方式播放背景音乐
    PlaySound(TEXT("D:\\backmusic.wav"), NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);

}

int main() {
    srand((unsigned)time(NULL));    //随机数种子
    initgraph(WIDTH, HEIGHT);       //创建绘图窗口
    BeginBatchDraw();               //开始批量绘图

    startBackgroundMusic();

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
    closegraph();
    return 0;
}

