/* ============================================================
   2048 en C avec raylib
   Compilation (Linux, raylib installé) :
       gcc 2048.c -o 2048 -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
   Compilation (macOS, via brew install raylib) :
       gcc 2048.c -o 2048 -lraylib -framework OpenGL -framework Cocoa \
           -framework IOKit -framework CoreVideo
   Compilation (Windows, MinGW + raylib) :
       gcc 2048.c -o 2048.exe -lraylib -lopengl32 -lgdi32 -lwinmm
   ============================================================ */

#include "raylib.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#define GRID_SIZE 4
#define CELL_SIZE 100
#define CELL_PADDING 12
#define BOARD_PADDING 20
#define TOP_BAR 130
#define WINDOW_W (GRID_SIZE * CELL_SIZE + (GRID_SIZE + 1) * CELL_PADDING + 2 * BOARD_PADDING)
#define WINDOW_H (WINDOW_W + TOP_BAR)

typedef enum { STATE_PLAYING, STATE_WON, STATE_LOST } GameState;

static int grid[GRID_SIZE][GRID_SIZE];
static int score = 0;
static int bestScore = 0;
static GameState state = STATE_PLAYING;
static bool wonMessageDismissed = false;

/* ---------- Logique du jeu ---------- */

void AddRandomTile(void) {
    int emptyCount = 0;
    int emptyCells[GRID_SIZE * GRID_SIZE][2];

    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            if (grid[r][c] == 0) {
                emptyCells[emptyCount][0] = r;
                emptyCells[emptyCount][1] = c;
                emptyCount++;
            }
        }
    }

    if (emptyCount == 0) return;

    int pick = GetRandomValue(0, emptyCount - 1);
    int value = (GetRandomValue(0, 9) == 0) ? 4 : 2; /* 10% de chances d'avoir un 4 */
    grid[emptyCells[pick][0]][emptyCells[pick][1]] = value;
}

void InitGame(void) {
    memset(grid, 0, sizeof(grid));
    score = 0;
    state = STATE_PLAYING;
    wonMessageDismissed = false;
    AddRandomTile();
    AddRandomTile();
}

/* Fait glisser et fusionner une ligne de 4 cases vers la gauche.
   Retourne true si la ligne a changé. */
bool SlideLineLeft(int line[GRID_SIZE]) {
    int original[GRID_SIZE];
    memcpy(original, line, sizeof(original));

    int compressed[GRID_SIZE] = {0};
    int idx = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        if (line[i] != 0) compressed[idx++] = line[i];
    }

    int merged[GRID_SIZE] = {0};
    int mIdx = 0;
    for (int i = 0; i < idx; i++) {
        if (i < idx - 1 && compressed[i] == compressed[i + 1] && compressed[i] != 0) {
            int value = compressed[i] * 2;
            merged[mIdx++] = value;
            score += value;
            if (value == 2048 && !wonMessageDismissed && state == STATE_PLAYING) {
                state = STATE_WON;
            }
            i++; /* on saute la case fusionnée */
        } else {
            merged[mIdx++] = compressed[i];
        }
    }

    memcpy(line, merged, sizeof(merged));
    return memcmp(original, line, sizeof(original)) != 0;
}

void ReverseLine(int line[GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE / 2; i++) {
        int tmp = line[i];
        line[i] = line[GRID_SIZE - 1 - i];
        line[GRID_SIZE - 1 - i] = tmp;
    }
}

bool MoveLeft(void) {
    bool moved = false;
    for (int r = 0; r < GRID_SIZE; r++) {
        if (SlideLineLeft(grid[r])) moved = true;
    }
    return moved;
}

bool MoveRight(void) {
    bool moved = false;
    for (int r = 0; r < GRID_SIZE; r++) {
        ReverseLine(grid[r]);
        bool changed = SlideLineLeft(grid[r]);
        ReverseLine(grid[r]);
        if (changed) moved = true;
    }
    return moved;
}

bool MoveUp(void) {
    bool moved = false;
    for (int c = 0; c < GRID_SIZE; c++) {
        int line[GRID_SIZE];
        for (int r = 0; r < GRID_SIZE; r++) line[r] = grid[r][c];
        bool changed = SlideLineLeft(line);
        for (int r = 0; r < GRID_SIZE; r++) grid[r][c] = line[r];
        if (changed) moved = true;
    }
    return moved;
}

bool MoveDown(void) {
    bool moved = false;
    for (int c = 0; c < GRID_SIZE; c++) {
        int line[GRID_SIZE];
        for (int r = 0; r < GRID_SIZE; r++) line[r] = grid[GRID_SIZE - 1 - r][c];
        bool changed = SlideLineLeft(line);
        for (int r = 0; r < GRID_SIZE; r++) grid[GRID_SIZE - 1 - r][c] = line[r];
        if (changed) moved = true;
    }
    return moved;
}

bool CanMove(void) {
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            if (grid[r][c] == 0) return true;
            if (c < GRID_SIZE - 1 && grid[r][c] == grid[r][c + 1]) return true;
            if (r < GRID_SIZE - 1 && grid[r][c] == grid[r + 1][c]) return true;
        }
    }
    return false;
}

/* ---------- Rendu ---------- */

Color GetTileColor(int value) {
    switch (value) {
        case 0:    return (Color){ 205, 193, 180, 255 };
        case 2:    return (Color){ 238, 228, 218, 255 };
        case 4:    return (Color){ 237, 224, 200, 255 };
        case 8:    return (Color){ 242, 177, 121, 255 };
        case 16:   return (Color){ 245, 149, 99, 255 };
        case 32:   return (Color){ 246, 124, 95, 255 };
        case 64:   return (Color){ 246, 94, 59, 255 };
        case 128:  return (Color){ 237, 207, 114, 255 };
        case 256:  return (Color){ 237, 204, 97, 255 };
        case 512:  return (Color){ 237, 200, 80, 255 };
        case 1024: return (Color){ 237, 197, 63, 255 };
        case 2048: return (Color){ 237, 194, 46, 255 };
        default:   return (Color){ 60, 58, 50, 255 };
    }
}

Color GetTextColor(int value) {
    return (value <= 4) ? (Color){ 119, 110, 101, 255 } : RAYWHITE;
}

void DrawBoard(void) {
    /* Fond du plateau */
    DrawRectangleRounded(
        (Rectangle){ BOARD_PADDING - 6, TOP_BAR - 6,
                     WINDOW_W - 2 * (BOARD_PADDING - 6), WINDOW_W - 2 * (BOARD_PADDING - 6) },
        0.03f, 8, (Color){ 187, 173, 160, 255 });

    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            int x = BOARD_PADDING + CELL_PADDING + c * (CELL_SIZE + CELL_PADDING);
            int y = TOP_BAR + CELL_PADDING + r * (CELL_SIZE + CELL_PADDING);
            int value = grid[r][c];

            DrawRectangleRounded((Rectangle){ x, y, CELL_SIZE, CELL_SIZE }, 0.1f, 8, GetTileColor(value));

            if (value != 0) {
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", value);
                int fontSize = (value < 100) ? 40 : (value < 1000) ? 34 : 26;
                int textW = MeasureText(buf, fontSize);
                DrawText(buf, x + (CELL_SIZE - textW) / 2, y + (CELL_SIZE - fontSize) / 2,
                          fontSize, GetTextColor(value));
            }
        }
    }
}

void DrawTopBar(void) {
    DrawText("2048", BOARD_PADDING, 20, 48, (Color){ 119, 110, 101, 255 });

    /* Score */
    int boxW = 110, boxH = 60;
    int scoreX = WINDOW_W - BOARD_PADDING - boxW * 2 - 10;
    int bestX = WINDOW_W - BOARD_PADDING - boxW;

    DrawRectangleRounded((Rectangle){ scoreX, 15, boxW, boxH }, 0.15f, 8, (Color){ 187, 173, 160, 255 });
    DrawText("SCORE", scoreX + 22, 22, 14, RAYWHITE);
    char scoreBuf[16];
    snprintf(scoreBuf, sizeof(scoreBuf), "%d", score);
    int sw = MeasureText(scoreBuf, 24);
    DrawText(scoreBuf, scoreX + (boxW - sw) / 2, 40, 24, RAYWHITE);

    DrawRectangleRounded((Rectangle){ bestX, 15, boxW, boxH }, 0.15f, 8, (Color){ 187, 173, 160, 255 });
    DrawText("MEILLEUR", bestX + 12, 22, 14, RAYWHITE);
    char bestBuf[16];
    snprintf(bestBuf, sizeof(bestBuf), "%d", bestScore);
    int bw = MeasureText(bestBuf, 24);
    DrawText(bestBuf, bestX + (boxW - bw) / 2, 40, 24, RAYWHITE);

    DrawText("Fleches pour jouer - R pour recommencer", BOARD_PADDING, 90, 16, (Color){ 119, 110, 101, 255 });
}

void DrawOverlay(const char *title, const char *subtitle) {
    DrawRectangle(BOARD_PADDING, TOP_BAR, WINDOW_W - 2 * BOARD_PADDING, WINDOW_W - 2 * BOARD_PADDING,
                  (Color){ 238, 228, 218, 180 });

    int tSize = 48;
    int tw = MeasureText(title, tSize);
    DrawText(title, (WINDOW_W - tw) / 2, WINDOW_H / 2 - 60, tSize, (Color){ 119, 110, 101, 255 });

    int sSize = 20;
    int sw = MeasureText(subtitle, sSize);
    DrawText(subtitle, (WINDOW_W - sw) / 2, WINDOW_H / 2, sSize, (Color){ 119, 110, 101, 255 });
}

int main(void) {
    InitWindow(WINDOW_W, WINDOW_H, "2048");
    SetTargetFPS(60);
    srand((unsigned int)time(NULL));
    SetRandomSeed((unsigned int)time(NULL));

    InitGame();

    while (!WindowShouldClose()) {
        /* --- Entrées --- */
        if (IsKeyPressed(KEY_R)) {
            InitGame();
        } else if (state == STATE_PLAYING) {
            bool moved = false;

            if (IsKeyPressed(KEY_LEFT))  moved = MoveLeft();
            else if (IsKeyPressed(KEY_RIGHT)) moved = MoveRight();
            else if (IsKeyPressed(KEY_UP))    moved = MoveUp();
            else if (IsKeyPressed(KEY_DOWN))  moved = MoveDown();

            if (moved) {
                AddRandomTile();
                if (score > bestScore) bestScore = score;
                if (state == STATE_PLAYING && !CanMove()) state = STATE_LOST;
            }
        } else if (state == STATE_WON && IsKeyPressed(KEY_ENTER)) {
            /* Continuer à jouer après avoir gagné */
            wonMessageDismissed = true;
            state = STATE_PLAYING;
        }

        /* --- Dessin --- */
        BeginDrawing();
        ClearBackground((Color){ 250, 248, 239, 255 });

        DrawTopBar();
        DrawBoard();

        if (state == STATE_WON) {
            DrawOverlay("Vous avez gagne !", "Entree pour continuer - R pour recommencer");
        } else if (state == STATE_LOST) {
            DrawOverlay("Partie terminee", "R pour recommencer");
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}