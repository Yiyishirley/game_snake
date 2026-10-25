#ifndef SNAKE_MAIN_H_
#define SNAKE_MAIN_H_

// ---- game flow ----
void gameInit();
void gameUpdate();
void gameOver();
void handleInput();

// ---- food & particles ----
bool placeFood();
void spawnParticles(int x, int y);
void updateParticles();

// ---- AI auto-play ----
int  aiChooseDirection();

// ---- drawing ----
void drawCell(int x, int y, COLORREF color, float inset);
void drawSnake();
void drawFood();
void drawParticles();
void drawWalls();
void drawHud();
void drawCenterText(const TCHAR* text, int centerY, int height);
void updateShake();

// ---- sound & high score ----
void startBackgroundMusic();
void stopSound();
void loadHighScore();
void saveHighScore();

#endif
