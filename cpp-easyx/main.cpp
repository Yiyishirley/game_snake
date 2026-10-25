// Snake (C++ / EasyX) - v2
// Controls: WASD or arrow keys to move, Space to pause, T to toggle AI auto-play,
//           Enter to restart after Game Over, Esc to quit.

#include <graphics.h>   // EasyX
#include <Windows.h>
#include <mmsystem.h>   // PlaySound
#include <conio.h>      // _kbhit / _getch
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <deque>
#include <fstream>
#include <vector>
#include "main.h"

#pragma comment(lib, "winmm.lib")   // PlaySound lives in winmm

// ===================== config =====================
const int WIDTH = 1080;
const int HEIGHT = 720;

const int CELL_SIZE = 30;                       // side of one grid cell
const int CELL_GAP = 2;                         // gap between cells
const int CELL_STEP = CELL_SIZE + CELL_GAP;
const int GRID_W = WIDTH / CELL_STEP;           // 33 columns
const int GRID_H = HEIGHT / CELL_STEP;          // 22 rows
const int OFFSET_X = WIDTH - GRID_W * CELL_STEP;   // leftover pixels, used as the border wall
const int OFFSET_Y = HEIGHT - GRID_H * CELL_STEP;

const COLORREF SNAKE_COLOR = RGB(255, 182, 193);
const COLORREF WALL_COLOR = RGB(255, 20, 147);
const COLORREF FOOD_COLOR = RGB(255, 255, 255);
const float HEAD_INSET = 0;                     // head is full size
const float TAIL_INSET = 10;                    // tail is 10px smaller

const int START_STEP_MS = 100;                  // one move every 100ms at the start
const int MIN_STEP_MS = 50;                     // fastest speed
const int SPEED_STEP_MS = 3;                    // each food makes the snake 3ms faster

const int PARTICLE_COUNT = 30;
const int PARTICLE_LIFE = 480;                  // frames
const float GRAVITY = 0.001f;
const int SHAKE_FRAMES = 120;
const int FRAME_SLEEP_MS = 4;                   // ~200 fps, and the CPU gets a break

const char* HIGH_SCORE_FILE = "highscore.txt";

// up, down, left, right
const int DX[4] = { 0, 0, -1, 1 };
const int DY[4] = { -1, 1, 0, 0 };
enum Dir { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

// ===================== state =====================
struct Point { int x, y; };

enum class GameState { Playing, Paused, Over, Won };

struct Snake {
    std::deque<Point> body;   // body[0] is the head
    int dir = DIR_RIGHT;
} snake;

struct Food {
    Point pos{ 0, 0 };
    int frame = -30;          // pop-in animation frame
    bool popping = true;
} food;

struct Particle {
    float x, y, dx, dy;
    float angle, spin;
    COLORREF color;
    int life;
};
std::vector<Particle> particles;

std::deque<int> inputQueue;   // buffered turns, at most 2
GameState state = GameState::Playing;
int score = 0;
int highScore = 0;
bool newRecord = false;
int stepMs = START_STEP_MS;
DWORD lastStepTick = 0;
bool aiOn = false;
bool aiUsed = false;          // AI runs don't count for the high score
int stepsSinceFood = 0;       // lets the AI notice when it is circling forever
int shakeFrames = SHAKE_FRAMES;
int cameraX = 0, cameraY = 0;

static bool samePoint(Point a, Point b) { return a.x == b.x && a.y == b.y; }
static bool inBounds(int x, int y) { return x >= 0 && x < GRID_W && y >= 0 && y < GRID_H; }
static int cellIndex(int x, int y) { return y * GRID_W + x; }

// ===================== sound & high score =====================
// The two WAV files are compiled into the exe as resources (see snake.rc).
void startBackgroundMusic() {
    PlaySound(TEXT("BACKMUSIC"), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC | SND_LOOP);
}
void stopSound() {
    PlaySound(NULL, NULL, 0);
}
void loadHighScore() {
    std::ifstream file(HIGH_SCORE_FILE);
    if (!(file >> highScore)) highScore = 0;
}
void saveHighScore() {
    std::ofstream file(HIGH_SCORE_FILE);
    if (file.is_open()) file << highScore;
}

// ===================== game flow =====================
void gameInit() {
    snake.body.clear();
    snake.body.push_back({ 2, 1 });
    snake.body.push_back({ 1, 1 });
    snake.body.push_back({ 0, 1 });
    snake.dir = DIR_RIGHT;
    inputQueue.clear();
    particles.clear();
    score = 0;
    stepMs = START_STEP_MS;          // speed resets on restart
    aiUsed = aiOn;
    stepsSinceFood = 0;
    newRecord = false;
    shakeFrames = SHAKE_FRAMES;
    placeFood();
    state = GameState::Playing;
    lastStepTick = GetTickCount();
    startBackgroundMusic();
}

void gameOver() {
    state = GameState::Over;
    if (!aiUsed && score > highScore) {
        highScore = score;
        newRecord = true;
        saveHighScore();
    }
    shakeFrames = 0;                 // shake once, not forever
    stopSound();
    PlaySound(TEXT("OUTPUT"), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
}

// Returns false when there is no free cell left (the player has filled the board).
bool placeFood() {
    std::vector<char> taken(GRID_W * GRID_H, 0);
    for (const Point& p : snake.body) taken[cellIndex(p.x, p.y)] = 1;
    std::vector<Point> freeCells;
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++)
            if (!taken[cellIndex(x, y)]) freeCells.push_back({ x, y });
    if (freeCells.empty()) return false;
    food.pos = freeCells[rand() % freeCells.size()];
    food.frame = -30;
    food.popping = true;
    return true;
}

static void queueDirection(int d) {
    if (state != GameState::Playing || aiOn || inputQueue.size() >= 2) return;
    int last = inputQueue.empty() ? snake.dir : inputQueue.back();
    if (DX[d] == -DX[last] && DY[d] == -DY[last]) return;   // no 180-degree turns
    if (d == last) return;
    inputQueue.push_back(d);
}

// Reads every key that is waiting, every frame, so quick double turns are not lost.
void handleInput() {
    while (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {              // arrow keys send two codes
            switch (_getch()) {
            case 72: queueDirection(DIR_UP); break;
            case 80: queueDirection(DIR_DOWN); break;
            case 75: queueDirection(DIR_LEFT); break;
            case 77: queueDirection(DIR_RIGHT); break;
            }
            continue;
        }
        switch (tolower(ch)) {                   // works with Caps Lock on
        case 'w': queueDirection(DIR_UP); break;
        case 's': queueDirection(DIR_DOWN); break;
        case 'a': queueDirection(DIR_LEFT); break;
        case 'd': queueDirection(DIR_RIGHT); break;
        case 't':
            aiOn = !aiOn;
            if (aiOn) aiUsed = true;
            inputQueue.clear();
            break;
        case ' ':
            if (state == GameState::Playing) { state = GameState::Paused; stopSound(); }
            else if (state == GameState::Paused) { state = GameState::Playing; lastStepTick = GetTickCount(); startBackgroundMusic(); }
            break;
        case 13:
            if (state == GameState::Over || state == GameState::Won) gameInit();
            break;
        case 27:
            stopSound();
            closegraph();
            exit(0);
        }
    }
}

void gameUpdate() {
    if (aiOn) snake.dir = aiChooseDirection();
    else if (!inputQueue.empty()) { snake.dir = inputQueue.front(); inputQueue.pop_front(); }

    Point head = snake.body.front();
    Point next = { head.x + DX[snake.dir], head.y + DY[snake.dir] };
    if (!inBounds(next.x, next.y)) { gameOver(); return; }

    bool eats = samePoint(next, food.pos);
    // The tail moves away this step unless the snake grows, so following your own tail is allowed.
    size_t checkCount = eats ? snake.body.size() : snake.body.size() - 1;
    for (size_t i = 0; i < checkCount; i++)
        if (samePoint(snake.body[i], next)) { gameOver(); return; }

    snake.body.push_front(next);
    if (!eats) { snake.body.pop_back(); stepsSinceFood++; return; }
    stepsSinceFood = 0;

    score += 10;
    if (stepMs - SPEED_STEP_MS >= MIN_STEP_MS) stepMs -= SPEED_STEP_MS;
    else stepMs = MIN_STEP_MS;
    shakeFrames = 0;
    spawnParticles(food.pos.x * CELL_STEP, food.pos.y * CELL_STEP);
    if (!placeFood()) {              // board is full: the player wins
        state = GameState::Won;
        if (!aiUsed && score > highScore) { highScore = score; newRecord = true; saveHighScore(); }
        stopSound();
    }
}

// ===================== AI: BFS to the food, only if the tail stays reachable =====================
static bool bfsPath(const std::deque<Point>& body, Point from, Point to, std::vector<Point>* path) {
    std::vector<char> blocked(GRID_W * GRID_H, 0);
    for (size_t i = 0; i + 1 < body.size(); i++) blocked[cellIndex(body[i].x, body[i].y)] = 1;   // tail is free
    blocked[cellIndex(to.x, to.y)] = 0;

    std::vector<int> prev(GRID_W * GRID_H, -1);
    int start = cellIndex(from.x, from.y), goal = cellIndex(to.x, to.y);
    if (start == goal) return false;
    std::vector<int> queue{ start };
    prev[start] = start;
    for (size_t qi = 0; qi < queue.size() && prev[goal] == -1; qi++) {
        int c = queue[qi], cx = c % GRID_W, cy = c / GRID_W;
        for (int d = 0; d < 4; d++) {
            int nx = cx + DX[d], ny = cy + DY[d];
            if (!inBounds(nx, ny)) continue;
            int n = cellIndex(nx, ny);
            if (prev[n] != -1 || blocked[n]) continue;
            prev[n] = c;
            queue.push_back(n);
        }
    }
    if (prev[goal] == -1) return false;
    if (path) {
        path->clear();
        for (int c = goal; c != start; c = prev[c]) path->insert(path->begin(), { c % GRID_W, c / GRID_W });
    }
    return true;
}

static int floodSize(const std::deque<Point>& body, Point from) {
    std::vector<char> seen(GRID_W * GRID_H, 0);
    for (size_t i = 0; i + 1 < body.size(); i++) seen[cellIndex(body[i].x, body[i].y)] = 1;
    std::vector<int> queue{ cellIndex(from.x, from.y) };
    seen[queue[0]] = 1;
    for (size_t qi = 0; qi < queue.size(); qi++) {
        int c = queue[qi], cx = c % GRID_W, cy = c / GRID_W;
        for (int d = 0; d < 4; d++) {
            int nx = cx + DX[d], ny = cy + DY[d];
            if (!inBounds(nx, ny) || seen[cellIndex(nx, ny)]) continue;
            seen[cellIndex(nx, ny)] = 1;
            queue.push_back(cellIndex(nx, ny));
        }
    }
    return (int)queue.size();
}

static int directionTo(Point a, Point b) {
    for (int d = 0; d < 4; d++) if (a.x + DX[d] == b.x && a.y + DY[d] == b.y) return d;
    return snake.dir;
}

int aiChooseDirection() {
    const std::deque<Point>& body = snake.body;
    Point head = body.front();

    // 1) Shortest path to the food, but only if the snake can still reach its tail after eating.
    std::vector<Point> toFood;
    if (bfsPath(body, head, food.pos, &toFood)) {
        std::deque<Point> after = body;
        for (const Point& p : toFood) {
            after.push_front(p);
            if (!samePoint(p, food.pos)) after.pop_back();
        }
        if ((int)after.size() >= GRID_W * GRID_H || bfsPath(after, after.front(), after.back(), nullptr))
            return directionTo(head, toFood[0]);
    }

    // 2) Otherwise pick the roomiest safe move, preferring moves that keep the tail reachable.
    //    If it has been circling for too long, add some randomness to break the loop.
    bool stuck = stepsSinceFood > GRID_W * GRID_H;
    int bestDir = snake.dir, bestScore = -1;
    for (int d = 0; d < 4; d++) {
        Point n = { head.x + DX[d], head.y + DY[d] };
        if (!inBounds(n.x, n.y)) continue;
        bool hitsBody = false;
        for (size_t i = 0; i + 1 < body.size(); i++) if (samePoint(body[i], n)) { hitsBody = true; break; }
        if (hitsBody) continue;
        std::deque<Point> after = body;
        after.push_front(n);
        after.pop_back();
        int s = (bfsPath(after, n, after.back(), nullptr) ? 10000 : 0)
              + floodSize(after, n) * 10
              + (stuck ? rand() % 2000 : abs(n.x - food.pos.x) + abs(n.y - food.pos.y));
        if (s > bestScore) { bestScore = s; bestDir = d; }
    }
    return bestDir;
}

// ===================== particles =====================
void spawnParticles(int x, int y) {
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        float angle = 2 * 3.14159265f * rand() / RAND_MAX;
        float speed = 0.5f * sqrtf(rand() / (float)RAND_MAX);
        Particle p;
        p.x = (float)(rand() % CELL_SIZE + x);
        p.y = (float)(rand() % CELL_SIZE + y);
        p.dx = speed * cosf(angle);
        p.dy = speed * sinf(angle);
        p.angle = (float)(rand() % 36000);
        p.spin = (rand() % 200 - 100) / 100.0f;
        p.color = RGB(rand() % 256, rand() % 256, rand() % 256);
        p.life = PARTICLE_LIFE;
        particles.push_back(p);
    }
}

void updateParticles() {
    for (size_t i = particles.size(); i-- > 0;) {
        Particle& p = particles[i];
        p.x += p.dx;
        p.y += p.dy;
        p.dy += GRAVITY;
        p.angle += p.spin;
        if (--p.life <= 0 || p.y > HEIGHT + 20) particles.erase(particles.begin() + i);
    }
}

// ===================== drawing =====================
void updateShake() {
    if (shakeFrames < SHAKE_FRAMES) {
        cameraX = rand() % 30 - 15;
        cameraY = rand() % 30 - 15;
        shakeFrames++;
    }
    else {
        cameraX = 0;
        cameraY = 0;
    }
}

// inset > 0 draws a smaller square, inset < 0 a bigger one (used by the food pop-in)
void drawCell(int x, int y, COLORREF color, float inset) {
    int half = (int)(inset / 2);
    int left = x * CELL_STEP + OFFSET_X / 2 + cameraX + half;
    int top = y * CELL_STEP + OFFSET_Y / 2 + cameraY + half;
    int right = x * CELL_STEP + CELL_SIZE + OFFSET_X / 2 + cameraX - half;
    int bottom = y * CELL_STEP + CELL_SIZE + OFFSET_Y / 2 + cameraY - half;
    setfillcolor(color);
    setlinecolor(color);
    fillrectangle(left, top, right, bottom);
}

void drawSnake() {
    int last = (int)snake.body.size() - 1;
    for (int i = 0; i <= last; i++) {
        float inset = (i == 0) ? HEAD_INSET : HEAD_INSET + (TAIL_INSET - HEAD_INSET) * i / last;
        drawCell(snake.body[i].x, snake.body[i].y, SNAKE_COLOR, inset);
    }
}

void drawFood() {
    if (food.popping) {
        if (food.frame > 20) {
            food.frame = -30;
            food.popping = false;
        }
        float inset = (float)(food.frame * food.frame / 10 - 100);
        drawCell(food.pos.x, food.pos.y, FOOD_COLOR, inset);
        food.frame++;
    }
    else {
        drawCell(food.pos.x, food.pos.y, FOOD_COLOR, 0);
    }
}

void drawParticles() {
    for (const Particle& p : particles) {
        float a = p.angle * 3.14159265f / 180.0f;
        int r = 5;
        int px = (int)(p.x + r * cosf(a));
        int py = (int)(p.y + r * sinf(a));
        setfillcolor(p.color);
        setlinecolor(p.color);
        fillrectangle(px - r, py - r, px + r, py + r);
    }
}

void drawWalls() {
    setfillcolor(WALL_COLOR);
    setlinecolor(WALL_COLOR);
    fillrectangle(cameraX, cameraY, OFFSET_X / 2 + cameraX, HEIGHT + cameraY);
    fillrectangle(WIDTH - OFFSET_X / 2 + cameraX, cameraY, WIDTH + cameraX, HEIGHT + cameraY);
    fillrectangle(cameraX, cameraY, WIDTH + cameraX, OFFSET_Y / 2 + cameraY);
    fillrectangle(cameraX, HEIGHT - OFFSET_Y / 2 + cameraY, WIDTH + cameraX, HEIGHT + cameraY);
}

void drawHud() {
    TCHAR buf[64];
    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    settextstyle(44, 0, _T("Consolas"));
    _stprintf_s(buf, _countof(buf), _T("Score: %d"), score);
    outtextxy(22, 14, buf);
    settextstyle(30, 0, _T("Consolas"));
    _stprintf_s(buf, _countof(buf), _T("High Score: %d"), highScore);
    outtextxy(22, 60, buf);
    _stprintf_s(buf, _countof(buf), _T("%dms%s"), stepMs, aiOn ? _T("  AI") : _T(""));
    outtextxy(WIDTH - 22 - textwidth(buf), 18, buf);
}

// Dark band behind the win text: the board is full of pink snake by then.
static void drawBand(int top, int bottom) {
    setfillcolor(RGB(14, 6, 11));
    solidrectangle(OFFSET_X / 2, top, WIDTH - OFFSET_X / 2, bottom);
    setfillcolor(WALL_COLOR);
    solidrectangle(OFFSET_X / 2, top, WIDTH - OFFSET_X / 2, top + 2);
    solidrectangle(OFFSET_X / 2, bottom - 2, WIDTH - OFFSET_X / 2, bottom);
}

void drawCenterText(const TCHAR* text, int centerY, int height) {
    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    settextstyle(height, 0, _T("Consolas"));   // width 0 = natural width, so the text fits
    RECT r = { 0, centerY - height, WIDTH, centerY + height };
    drawtext(text, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

// ===================== main =====================
int main() {
    srand((unsigned)time(NULL));
    timeBeginPeriod(1);               // 1ms timer resolution so Sleep() is accurate
    initgraph(WIDTH, HEIGHT);
    BeginBatchDraw();
    loadHighScore();
    gameInit();

    while (true) {
        handleInput();

        DWORD now = GetTickCount();
        if (state == GameState::Playing && now - lastStepTick > (DWORD)stepMs) {
            lastStepTick = now;
            gameUpdate();
        }
        if (state != GameState::Paused) updateParticles();
        updateShake();

        cleardevice();
        drawParticles();
        drawFood();
        drawSnake();
        drawWalls();
        drawHud();

        TCHAR buf[64];
        switch (state) {
        case GameState::Paused:
            drawCenterText(_T("Pause! ^3^"), HEIGHT / 2, 90);
            break;
        case GameState::Over:
            drawCenterText(_T("Game Over (-3-) ~ Press Enter to Restart"), HEIGHT / 2, 50);
            if (newRecord) {
                _stprintf_s(buf, _countof(buf), _T("New High Score: %d"), highScore);
                drawCenterText(buf, HEIGHT / 2 + 70, 40);
            }
            break;
        case GameState::Won:
            drawBand(HEIGHT / 2 - 150, HEIGHT / 2 + 150);
            settextcolor(SNAKE_COLOR);
            settextstyle(110, 0, _T("Consolas"));
            {
                RECT r = { 0, HEIGHT / 2 - 115, WIDTH, HEIGHT / 2 + 5 };
                drawtext(_T("YOU WIN!"), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            _stprintf_s(buf, _countof(buf), _T("The board is full ^o^   Score: %d"), score);
            drawCenterText(buf, HEIGHT / 2 + 35, 36);
            drawCenterText(_T("Press Enter to Play Again"), HEIGHT / 2 + 110, 28);
            break;
        default:
            break;
        }

        FlushBatchDraw();
        Sleep(FRAME_SLEEP_MS);        // the old loop never slept and used 100% of a CPU core
    }
}
