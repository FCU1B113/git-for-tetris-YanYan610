#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>
#include <conio.h>  // 為了使用 _getch 與 _kbhit

#define CANVAS_WIDTH 10
#define CANVAS_HEIGHT 20

#define FALL_DELAY 500
#define RENDER_DELAY 100

#define LEFT_KEY 0x25
#define RIGHT_KEY 0x27
#define ROTATE_KEY 0x26
#define DOWN_KEY 0x28
#define FALL_KEY 0x20
#define HOLD_KEY 'C'
#define PAUSE_KEY 'P'

#define LEFT_FUNC() (GetAsyncKeyState(LEFT_KEY) & 0x8000)
#define RIGHT_FUNC() (GetAsyncKeyState(RIGHT_KEY) & 0x8000)
#define ROTATE_FUNC() (GetAsyncKeyState(ROTATE_KEY) & 0x8000)
#define DOWN_FUNC() (GetAsyncKeyState(DOWN_KEY) & 0x8000)
#define FALL_FUNC() (GetAsyncKeyState(FALL_KEY) & 0x8000)
#define HOLD_FUNC() (GetAsyncKeyState(HOLD_KEY) & 0x8000)
#define PAUSE_FUNC() (GetAsyncKeyState(PAUSE_KEY) & 0x8000)

typedef enum
{
    RED = 41,
    GREEN,
    YELLOW,
    BLUE,
    PURPLE,
    CYAN,
    WHITE,
    BLACK = 0,
} Color;

typedef enum
{
    EMPTY = -1,
    I,
    J,
    L,
    O,
    S,
    T,
    Z
} ShapeId;

typedef struct {
    ShapeId shape;
    Color color;
    int size;
    char rotates[4][4][4];
} Shape;

typedef struct {
    int x, y;
    int score;
    int rotate;
    int fallTime;
    ShapeId queue[4];
    ShapeId holdPiece;
    bool holdUsed;
    int level;
    int fallDelay;
    int lastClear;
    int highScore;
} State;

typedef struct {
    Color color;
    ShapeId shape;
    bool current;
} Block;

Shape shapes[7] = {
    {.shape = I,
     .color = CYAN,
     .size = 4,
     .rotates =
         {
             {{0, 0, 0, 0},
              {1, 1, 1, 1},
              {0, 0, 0, 0},
              {0, 0, 0, 0}},
             {{0, 0, 1, 0},
              {0, 0, 1, 0},
              {0, 0, 1, 0},
              {0, 0, 1, 0}},
             {{0, 0, 0, 0},
              {0, 0, 0, 0},
              {1, 1, 1, 1},
              {0, 0, 0, 0}},
             {{0, 1, 0, 0},
              {0, 1, 0, 0},
              {0, 1, 0, 0},
              {0, 1, 0, 0}}}},
    {.shape = J,
     .color = BLUE,
     .size = 3,
     .rotates =
         {
             {{1, 0, 0},
              {1, 1, 1},
              {0, 0, 0}},
             {{0, 1, 1},
              {0, 1, 0},
              {0, 1, 0}},
             {{0, 0, 0},
              {1, 1, 1},
              {0, 0, 1}},
             {{0, 1, 0},
              {0, 1, 0},
              {1, 1, 0}}}},
    {.shape = L,
     .color = YELLOW,
     .size = 3,
     .rotates =
         {
             {{0, 0, 1},
              {1, 1, 1},
              {0, 0, 0}},
             {{0, 1, 0},
              {0, 1, 0},
              {0, 1, 1}},
             {{0, 0, 0},
              {1, 1, 1},
              {1, 0, 0}},
             {{1, 1, 0},
              {0, 1, 0},
              {0, 1, 0}}}},
    {.shape = O,
     .color = WHITE,
     .size = 2,
     .rotates =
         {
             {{1, 1},
              {1, 1}},
             {{1, 1},
              {1, 1}},
             {{1, 1},
              {1, 1}},
             {{1, 1},
              {1, 1}}}},
    {.shape = S,
     .color = GREEN,
     .size = 3,
     .rotates =
         {
             {{0, 1, 1},
              {1, 1, 0},
              {0, 0, 0}},
             {{0, 1, 0},
              {0, 1, 1},
              {0, 0, 1}},
             {{0, 0, 0},
              {0, 1, 1},
              {1, 1, 0}},
             {{1, 0, 0},
              {1, 1, 0},
              {0, 1, 0}}}},
    {.shape = T,
     .color = PURPLE,
     .size = 3,
     .rotates =
         {
             {{0, 1, 0},
              {1, 1, 1},
              {0, 0, 0}},

             {{0, 1, 0},
              {0, 1, 1},
              {0, 1, 0}},
             {{0, 0, 0},
              {1, 1, 1},
              {0, 1, 0}},
             {{0, 1, 0},
              {1, 1, 0},
              {0, 1, 0}}}},
    {.shape = Z,
     .color = RED,
     .size = 3,
     .rotates =
         {
             {{1, 1, 0},
              {0, 1, 1},
              {0, 0, 0}},
             {{0, 0, 1},
              {0, 1, 1},
              {0, 1, 0}},
             {{0, 0, 0},
              {1, 1, 0},
              {0, 1, 1}},
             {{0, 1, 0},
              {1, 1, 0},
              {1, 0, 0}}}},
};

void setBlock(Block* block, Color color, ShapeId shape, bool current)
{
    block->color = color;
    block->shape = shape;
    block->current = current;
}

void resetBlock(Block* block)
{
    block->color = BLACK;
    block->shape = EMPTY;
    block->current = false;
}
bool move(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], int originalX, int originalY, int originalRotate, int newX, int newY, int newRotate, ShapeId shapeId) {
    Shape shapeData = shapes[shapeId];
    int size = shapeData.size;

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (shapeData.rotates[newRotate][i][j]) {
                if (newX + j < 0 || newX + j >= CANVAS_WIDTH || newY + i < 0 || newY + i >= CANVAS_HEIGHT)
                    return false;
                if (!canvas[newY + i][newX + j].current && canvas[newY + i][newX + j].shape != EMPTY)
                    return false;
            }
        }
    }

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (shapeData.rotates[originalRotate][i][j])
                resetBlock(&canvas[originalY + i][originalX + j]);
        }
    }

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (shapeData.rotates[newRotate][i][j])
                setBlock(&canvas[newY + i][newX + j], shapeData.color, shapeId, true);
        }
    }

    return true;
}

int clearLine(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH]) {
    for (int i = 0; i < CANVAS_HEIGHT; i++) {
        for (int j = 0; j < CANVAS_WIDTH; j++) {
            if (canvas[i][j].current) {
                canvas[i][j].current = false;
            }
        }
    }
    int linesCleared = 0;
    for (int i = CANVAS_HEIGHT - 1; i >= 0; i--) {
        bool isFull = true;
        for (int j = 0; j < CANVAS_WIDTH; j++) {
            if (canvas[i][j].shape == EMPTY) {
                isFull = false;
                break;
            }
        }
        if (isFull) {
            linesCleared++;
            for (int j = i; j > 0; j--) {
                for (int k = 0; k < CANVAS_WIDTH; k++) {
                    canvas[j][k] = canvas[j - 1][k];
                }
            }
            i++;
        }
    }
    return linesCleared;
}

void printCanvas(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State* state) {
    printf("\033[0;0H\n");
    for (int i = 0; i < CANVAS_HEIGHT; i++) {
        printf("|");
        for (int j = 0; j < CANVAS_WIDTH; j++) {
            printf("\033[%dm　", canvas[i][j].color);
        }
        printf("\033[0m|\n");
    }
    printf("\033[%d;%dHNext:", 3, CANVAS_WIDTH * 2 + 5);
    for (int i = 1; i <= 3; i++) {
        Shape shapeData = shapes[state->queue[i]];
        for (int j = 0; j < 4; j++) {
            printf("\033[%d;%dH", i * 4 + j, CANVAS_WIDTH * 2 + 15);
            for (int k = 0; k < 4; k++) {
                if (j < shapeData.size && k < shapeData.size && shapeData.rotates[0][j][k]) {
                    printf("\x1b[%dm  ", shapeData.color);
                }
                else {
                    printf("\x1b[0m  ");
                }
            }
        }
    }
    printf("\033[%d;%dHHold:", 2, CANVAS_WIDTH * 2 + 5);
    Shape holdShape = shapes[state->holdPiece];
    for (int i = 0; i < 4; i++) {
        printf("\033[%d;%dH", i + 3, CANVAS_WIDTH * 2 + 5);
        for (int j = 0; j < 4; j++) {
            if (i < holdShape.size && j < holdShape.size && holdShape.rotates[0][i][j]) {
                printf("\x1b[%dm  ", holdShape.color);
            }
            else {
                printf("\x1b[0m  ");
            }
        }
    }
    printf("\033[%d;%dHScore: %d", 15, CANVAS_WIDTH * 2 + 5, state->score);
    printf("\033[%d;%dHLevel: %d", 16, CANVAS_WIDTH * 2 + 5, state->level);

    if (state->lastClear == 4) {
        printf("\033[%d;%dH\x1b[33mTETRIS!!\x1b[0m", CANVAS_HEIGHT + 2, 2);
    }

}

void logic(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State* state) {
    if (HOLD_FUNC() && !state->holdUsed) {
        ShapeId temp = state->holdPiece;
        state->holdPiece = state->queue[0];
        state->holdUsed = true;
        if (temp == EMPTY) {
            state->queue[0] = state->queue[1];
            state->queue[1] = state->queue[2];
            state->queue[2] = state->queue[3];
            state->queue[3] = rand() % 7;
        }
        else {
            state->queue[0] = temp;
        }
        state->x = CANVAS_WIDTH / 2;
        state->y = 0;
        state->rotate = 0;
        for (int i = 0; i < CANVAS_HEIGHT; i++)
            for (int j = 0; j < CANVAS_WIDTH; j++)
                if (canvas[i][j].current)
                    resetBlock(&canvas[i][j]);
        Shape shapeData = shapes[state->queue[0]];
        for (int i = 0; i < shapeData.size; i++)
            for (int j = 0; j < shapeData.size; j++)
                if (shapeData.rotates[0][i][j])
                    setBlock(&canvas[state->y + i][state->x + j], shapeData.color, state->queue[0], true);
        return;
    }
    if (ROTATE_FUNC()) {
        int newRotate = (state->rotate + 1) % 4;
        if (move(canvas, state->x, state->y, state->rotate, state->x, state->y, newRotate, state->queue[0]))
            state->rotate = newRotate;
    }
    else if (LEFT_FUNC()) {
        if (move(canvas, state->x, state->y, state->rotate, state->x - 1, state->y, state->rotate, state->queue[0]))
            state->x -= 1;
    }
    else if (RIGHT_FUNC()) {
        if (move(canvas, state->x, state->y, state->rotate, state->x + 1, state->y, state->rotate, state->queue[0]))
            state->x += 1;
    }
    else if (DOWN_FUNC()) {
        state->fallTime = state->fallDelay;
    }
    else if (FALL_FUNC()) {
        state->fallTime += state->fallDelay * CANVAS_HEIGHT;
    }

    state->fallTime += RENDER_DELAY;
    int newLevel = state->score / 10;
    if (newLevel > state->level) {
        state->level = newLevel;
        state->fallDelay = FALL_DELAY - state->level * 20;
        if (state->fallDelay < 100)
            state->fallDelay = 100;
    }
    while (state->fallTime >= state->fallDelay) {
        state->fallTime -= state->fallDelay;
        if (move(canvas, state->x, state->y, state->rotate, state->x, state->y + 1, state->rotate, state->queue[0])) {
            state->y++;
        }
        else {
            int cleared = clearLine(canvas);
            state->lastClear = cleared;

            if (cleared == 1)
                state->score += 1;
            else if (cleared == 2)
                state->score += 3;
            else if (cleared == 3)
                state->score += 5;
            else if (cleared == 4)
                state->score += 8; // Tetris 加倍

            state->x = CANVAS_WIDTH / 2;
            state->y = 0;
            state->rotate = 0;
            state->fallTime = 0;
            state->holdUsed = false;
            state->queue[0] = state->queue[1];
            state->queue[1] = state->queue[2];
            state->queue[2] = state->queue[3];
            state->queue[3] = rand() % 7;
            if (!move(canvas, state->x, state->y, state->rotate, state->x, state->y, state->rotate, state->queue[0])) {
                printf("\033[%d;%dH\x1b[41m GAME OVER! \x1b[0m", CANVAS_HEIGHT / 2, CANVAS_WIDTH);
                printf("\033[%d;%dH按 R 重新開始遊戲...", CANVAS_HEIGHT / 2 + 2, CANVAS_WIDTH);
                while (1) {
                    if (_kbhit()) {
                        char key = _getch();
                        if (key == 'r' || key == 'R') {
                            system("cls");
                            return main(); // 重新開始遊戲
                        }
                    }
                    Sleep(100);
                }

            }
        }
    }
}

int main() {
    // 顯示開始畫面
    printf("\033[0;0H");
    printf("\n\n\n\t\t\x1b[36mTETRIS 遊戲開始\x1b[0m\n\n");
    printf("\t\t按任意鍵開始...\n");
    while (!_kbhit()) {
        Sleep(50);
    }
    _getch();
    system("cls");

    srand(time(NULL));
    State state = {
        .x = CANVAS_WIDTH / 2,
        .y = 0,
        .score = 0,
        .rotate = 0,
        .fallTime = 0,
        .holdPiece = EMPTY,
        .holdUsed = false,
        .level = 0,
        .fallDelay = FALL_DELAY,
        .lastClear = 0,
        .highScore = 0
    };
    for (int i = 0; i < 4; i++)
        state.queue[i] = rand() % 7;
    Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH];
    for (int i = 0; i < CANVAS_HEIGHT; i++)
        for (int j = 0; j < CANVAS_WIDTH; j++)
            resetBlock(&canvas[i][j]);
    Shape shapeData = shapes[state.queue[0]];
    for (int i = 0; i < shapeData.size; i++)
        for (int j = 0; j < shapeData.size; j++)
            if (shapeData.rotates[0][i][j])
                setBlock(&canvas[state.y + i][state.x + j], shapeData.color, state.queue[0], true);

    bool paused = false;
    while (1) {
        if (PAUSE_FUNC()) {
            paused = !paused;
            Sleep(200); // 防止連續切換
        }
        if (!paused) {
            printCanvas(canvas, &state);
            logic(canvas, &state);
        }
        else {
            printf("\033[%d;%dH\x1b[33m[暫停中] 按 P 繼續\x1b[0m", CANVAS_HEIGHT / 2, CANVAS_WIDTH);
        }
        Sleep(100);
    }
    return 0;
}
