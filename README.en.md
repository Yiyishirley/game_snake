# Snake

[中文](README.md)

![AI auto-play demo](docs/demo.gif)

**Play online:** https://yiyishirley.github.io/game_snake/

A pink pixel-style Snake game that started as a C++ / EasyX desktop game and was later ported to the web. Eating food sets off particles and a screen shake, the snake speeds up as it grows, and pressing `T` hands control to an AI.

## Features

- **Three difficulty levels**: Easy / Normal / Hard start at 150 / 100 / 70 ms per step, 3 ms faster per food
- **Obstacle mode**: random wall blocks, toggled with `U` in the menu
- **AI auto-play**: BFS pathfinding with a safety check; it can fill the whole 33×22 board
- **AI thinking view**: press `V` to see the BFS search area and the chosen route live; when eating would trap the snake, the rejected route is shown and the AI switches to following its tail
- **Special food**: a time-limited gold food worth +50, and a blue food that slows the snake down
- **Combo**: eating again quickly multiplies the points, up to ×4
- **Share card**: after Game Over, generate a portrait image with the score and the final board
- **Stats**: games played, average score, best combo, play time, and causes of death (wall / self / obstacle)
- **High score and local leaderboard**: localStorage on the web, `highscore.txt` in C++; AI runs are not recorded
- **Original music**: background track and Game Over sound
- **Controls**: WASD or arrow keys, works with Caps Lock on; swipe on phones; pauses automatically when you switch tabs
- **Chinese / English**: one-click switch in the top-right corner; follows the browser language and remembers your choice
- **Win state** when the board is full

## Controls

| Key | Action |
|---|---|
| `W A S D` / arrows | Move |
| `Space` | Pause / resume |
| `T` | Toggle AI auto-play |
| `V` | Show the AI's thinking (web) |
| `Enter` | Start / restart after Game Over |
| `1` `2` `3`, `U` | Pick difficulty, toggle obstacles (web menu) |
| `Esc` | Back to menu (web) / quit (C++) |

## History

| Version | Date | What changed |
|---|---|---|
| v1 | Apr 2024 | C++ + EasyX base game: grid map, tapering snake body, food pop-in animation, screen shake, particles, score |
| v1.1 | Jun 2024 | High score, pause, background music and Game Over sound; packaged as a Windows installer with Visual Studio |
| v2 | Oct 2026 | Bug fixes and renaming; web port (HTML5 Canvas); difficulty levels, obstacles, AI auto-play, leaderboard, touch controls |
| v2.1 | Oct 2026 | Web: AI thinking view, special food, combos, share card, stats, eat sounds |

The original code is kept in [`original/`](original/), the v2 C++ version is in [`cpp-easyx/`](cpp-easyx/), and the web version is `index.html` at the root.

## Bugs fixed in v2

| Problem | Cause | Fix |
|---|---|---|
| Following your own tail killed you | Self-collision also checked the cell the tail had just left (`i < snack_len + 1`) | Skip the tail cell unless the snake grows this step |
| One CPU core at 100% | Main, pause and Game Over loops never slept | `Sleep(4)` per frame with `timeBeginPeriod(1)`, about 200 fps |
| Fast double turns were lost or delayed | Only one key was read per movement step | Read every key each frame, buffer up to 2 turns, reject 180° reversals |
| Filling the board froze the game | Food placement retried random cells with `goto` forever | Collect free cells first, pick one; no free cell means a win |
| Caps Lock or arrow keys did nothing | `_getch()` only matched lowercase `wasd` | Lowercase every key and handle the two-byte arrow codes (224 + 72/80/75/77) |
| Game Over text ran off screen | 50 px per character × 40 characters = 2000 px > 1080 px | Use natural character width so the line fits |
| Game Over screen shook forever | The shake counter was reset every frame in that loop | Shake once when the game ends |
| Particles never went away | No lifetime, so off-screen particles were updated forever | Give each particle a lifetime and remove it when it expires or falls off screen |
| Speed never changed | `#define GAMETIME 100` was a constant | Make it a variable: 3 ms faster per food, capped, reset on restart |
| Unclear names | `snack`, `gameupdata`, `gamedrawpixal`, `lizi` | Renamed to `snake`, `gameUpdate`, `drawCell`, `particle` and so on |

## How the AI plays

Before every step:

1. Find the shortest path from the head to the food with **BFS**.
2. Simulate the snake after following that path and eating, then check with BFS that **the new head can still reach its own tail**. If it can, eating will not trap the snake, so take the path.
3. If the path is unsafe or missing, pick a move that does not collide, preferring moves that **keep the tail reachable** and leave the **most open space** (flood-fill count), and circle until a safe chance appears.
4. If it has circled too long without eating, add some randomness to break the loop.

Press `V` in the web version to watch this happen: light blue cells are the BFS search area, the blue line is the safe route to the food, the red dashed line is a rejected route (eating there would trap the snake), and the yellow line is the route back to the tail.

In testing on an empty board, the AI filled all 726 cells in 2 of 3 runs.

## Running it

**Web**: open `index.html` in a browser. To publish on GitHub Pages, go to Settings → Pages and serve the `main` branch root.

**C++** (Windows + Visual Studio + [EasyX](https://easyx.cn)):

1. Create an empty project and add `main.cpp`, `main.h`, `snake.rc` and the two `.wav` files from `cpp-easyx/`
2. Build and run in **Release / x64**
3. The sounds are compiled into the exe as resources; `winmm.lib` is linked from the code with `#pragma comment`

## Development notes

I wrote v1 on my own in C++. For the v2 port and refactor I used AI-assisted coding (Claude): I set the requirements, tested the game, found the bugs and chose the fixes, and the AI helped write the code.
