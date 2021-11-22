#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

typedef struct KeyStates
{
    bool r;
    bool space;
    bool escape;
    bool up_arrow;
    bool down_arrow;
    bool left_arrow;
    bool right_arrow;
    bool kp1;
    bool kp2;
    bool kp3;
    bool kpplus;
    bool kpminus;
}
KeyStates;

void keystates_reset(KeyStates *states)
{
    states->r = false;
    states->space = false;
    states->escape = false;
    states->up_arrow = false;
    states->down_arrow = false;
    states->left_arrow = false;
    states->right_arrow = false;
    states->kp1 = false;
    states->kp2 = false;
    states->kp3 = false;
    states->kpplus = false;
    states->kpminus = false;
}

static KeyStates keysPressed;

int DEFAULT_WINDOW_WIDTH = 1280;
int DEFAULT_WINDOW_HEIGHT = 1024;

const int MIN_CELL_SIZE = 4;
const int MAX_CELL_SIZE = 64;
const int CELL_SIZE_STEP = 2;
int CELL_SIZE = 16;

const int DELAY_MS_FAST = 25;
const int DELAY_MS_SLOW = 50;
const int DELAY_MS_SLOWEST = 500;
int DELAY_MS = DELAY_MS_FAST;

static bool GAME_RUNNING = true;
static bool FORCE_QUIT = false;
static bool GAME_SHOULD_UPDATE = true;

static bool FLAG_WINDOW_DIMENSIONS_CHANGED = true;

const int CANVAS_OFFSET_STEP = 4;
static int CANVAS_OFFSET_X = 0;
static int CANVAS_OFFSET_Y = 0;

static int MOUSE_CLICK_POS_X = 0;
static int MOUSE_CLICK_POS_Y = 0;
static bool FLAG_MOUSE_CLICKED = false;

bool inRange(int value, int min, int max)
{
    return value >= min && value < max;
}

int mapToIndex(int numCols, int row, int col)
{
    return (row * numCols) + col;
}

int GOL_CalcNeighbors(int index, int rows, int columns, bool *cells)
{
    int row = index / rows;
    int column = index % columns;
    int rowAbove = row - 1;
    int rowBelow = row + 1;
    int columnLeft = column - 1;
    int columnRight = column + 1;

    bool top = inRange(rowAbove, 0, rows);
    bool bottom = inRange(rowBelow, 0, rows);
    bool left = inRange(columnLeft, 0, columns);
    bool right = inRange(columnRight, 0, columns);

    int liveNeighbors = 0;
    int i = 0;
    i = mapToIndex(columns, rowAbove, columnLeft);
    liveNeighbors += (top && left) * cells[i];
    i = mapToIndex(columns, rowAbove, column);
    liveNeighbors += top * cells[i];
    i = mapToIndex(columns, rowAbove, columnRight);
    liveNeighbors += (top && right) * cells[i];
    i = mapToIndex(columns, row, columnLeft);
    liveNeighbors += left * cells[i];
    i = mapToIndex(columns, row, columnRight);
    liveNeighbors += right * cells[i];
    i = mapToIndex(columns, rowBelow, columnLeft);
    liveNeighbors += (bottom && left) * cells[i];
    i = mapToIndex(columns, rowBelow, column);
    liveNeighbors += bottom * cells[i];
    i = mapToIndex(columns, rowBelow, columnRight);
    liveNeighbors += (bottom && right) * cells[i];
    return liveNeighbors;
}

void GOL_Update(int rows, int columns, bool *cells)
{
    int cellCount = rows * columns;
    bool nextGen[cellCount];

    for (int i = 0; i < cellCount; i++)
    {
        int liveNeighbors = GOL_CalcNeighbors(i, rows, columns, cells);
        nextGen[i] = (cells[i] && liveNeighbors == 2) || liveNeighbors == 3;
    }

    for (int i = 0; i < cellCount; i++)
    {
        cells[i] = nextGen[i];
    }
}

typedef struct GameOfLife
{
    bool running;
    bool shouldUpdate;
    int canvasRows;
    int canvasColumns; 
    SDL_Rect canvasBackgroundRect;
    int numVerticalGridLines;
    float verticalGridLineSpacing;
    int numHorizontalGridLines;
    float horizontalGridLineSpacing;
    bool *cells;
    int cellCount;
    int cellSize;
    int windowWidth;
    int windowHeight;
}
GameOfLife;

bool gol_init(GameOfLife *game, int windowWidth, int windowHeight)
{
    game->running = true;
    game->shouldUpdate = true;
    game->windowWidth = windowWidth;
    game->windowHeight = windowHeight;
    game->cellSize = CELL_SIZE;
    game->canvasRows = 1000;
    game->canvasColumns = 1000;
    game->canvasBackgroundRect.w = game->canvasColumns * game->cellSize;
    game->canvasBackgroundRect.h = game->canvasRows * game->cellSize;
    game->canvasBackgroundRect.x = (windowWidth / 2) - (game->canvasBackgroundRect.w / 2);
    game->canvasBackgroundRect.y = (windowHeight / 2) - (game->canvasBackgroundRect.h / 2);
    game->numVerticalGridLines = game->canvasBackgroundRect.w / game->cellSize;
    game->verticalGridLineSpacing = game->canvasBackgroundRect.w / game->numVerticalGridLines;
    game->numHorizontalGridLines = game->canvasBackgroundRect.h / game->cellSize;
    game->horizontalGridLineSpacing = game->canvasBackgroundRect.h / game->numHorizontalGridLines;
    game->cellCount = game->numVerticalGridLines * game->numHorizontalGridLines;
    game->cells = calloc(game->cellCount, sizeof(bool));
    for (int i = 0; i < game->cellCount; i++)
    {
        game->cells[i] = false;
    }
}

void gol_recalculate_canvas_dimensions(GameOfLife *game)
{
    game->canvasBackgroundRect.w = game->canvasColumns * game->cellSize;
    game->canvasBackgroundRect.h = game->canvasRows * game->cellSize;
    game->canvasBackgroundRect.x = (game->windowWidth / 2) - (game->canvasBackgroundRect.w / 2) + CANVAS_OFFSET_X;
    game->canvasBackgroundRect.y = (game->windowHeight / 2) - (game->canvasBackgroundRect.h / 2) + CANVAS_OFFSET_Y;
    game->numVerticalGridLines = game->canvasBackgroundRect.w / game->cellSize;
    game->verticalGridLineSpacing = game->canvasBackgroundRect.w / game->numVerticalGridLines;
    game->numHorizontalGridLines = game->canvasBackgroundRect.h / game->cellSize;
    game->horizontalGridLineSpacing = game->canvasBackgroundRect.h / game->numHorizontalGridLines;
}

void gol_window_resized(GameOfLife *game, int windowWidth, int windowHeight)
{
    game->windowWidth = windowWidth;
    game->windowHeight = windowHeight;
    gol_recalculate_canvas_dimensions(game);
}

void gol_reset_cells(GameOfLife *game)
{
    for (int i = 0; i < game->cellCount; i++)
    {
        game->cells[i] = false;
    }
}

void gol_mouse_clicked(GameOfLife *game)
{
    bool inCanvasBoundaries = MOUSE_CLICK_POS_X >= game->canvasBackgroundRect.x && 
        MOUSE_CLICK_POS_X <= game->canvasBackgroundRect.x + game->canvasBackgroundRect.w &&
        MOUSE_CLICK_POS_Y >= game->canvasBackgroundRect.y && 
        MOUSE_CLICK_POS_Y <= game->canvasBackgroundRect.y + game->canvasBackgroundRect.h;

    if (inCanvasBoundaries)
    {
        int relativeX = MOUSE_CLICK_POS_X - game->canvasBackgroundRect.x;
        int relativeY = MOUSE_CLICK_POS_Y - game->canvasBackgroundRect.y;
        int row = relativeY / game->horizontalGridLineSpacing;
        int col = relativeX / game->verticalGridLineSpacing;
        int index = mapToIndex(game->numVerticalGridLines, row, col);
        game->cells[index] = !game->cells[index];
    }
}

void gol_render(GameOfLife *game, SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
    SDL_RenderFillRect(renderer, &game->canvasBackgroundRect);

    SDL_SetRenderDrawColor(renderer, 0, 128, 128, 128);
    SDL_FRect cellRect;
    cellRect.w = game->horizontalGridLineSpacing;
    cellRect.h = game->verticalGridLineSpacing;
    cellRect.x = game->canvasBackgroundRect.x;
    cellRect.y = game->canvasBackgroundRect.y;
    for (int row = 0; row < game->numHorizontalGridLines; row++)
    {
        for (int col = 0; col < game->numVerticalGridLines; col++)
        {
            int cellIndex = mapToIndex(game->numVerticalGridLines, row, col);
            if (game->cells[cellIndex])
            {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            } 
            else
            {
                SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            }
            SDL_RenderFillRectF(renderer, &cellRect);
            cellRect.x += cellRect.w;
        }
        cellRect.x = game->canvasBackgroundRect.x;
        cellRect.y += cellRect.h;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 128);
    for (int i = 0; i <= game->numVerticalGridLines; i++)
    {
        float x = game->canvasBackgroundRect.x + (i * game->verticalGridLineSpacing);
        SDL_RenderDrawLine(renderer, x, game->canvasBackgroundRect.y, x, game->canvasBackgroundRect.y + game->canvasBackgroundRect.h);
    }
    for (int i = 0; i <= game->numHorizontalGridLines; i++)
    {
        float y = game->canvasBackgroundRect.y + (i * game->horizontalGridLineSpacing);
        SDL_RenderDrawLine(renderer, game->canvasBackgroundRect.x, y, game->canvasBackgroundRect.x + game->canvasBackgroundRect.w, y);
    }
}

void gol_update(GameOfLife *game, KeyStates *pressedKeys)
{
    bool recalcCanvasDimensions = false;

    if (pressedKeys->r)
    {
        gol_reset_cells(game);
    }

    if (pressedKeys->escape)
    {
        game->running = false;
        return; // Return immediately
    }

    if (pressedKeys->space)
    {
        // Toggle should update
        game->shouldUpdate = !game->shouldUpdate;
    }

    if (pressedKeys->kp1)
    {
        DELAY_MS = DELAY_MS_FAST;
    }
    else if (pressedKeys->kp2)
    {
        DELAY_MS = DELAY_MS_SLOW;
    }
    else if (pressedKeys->kp3)
    {
        DELAY_MS = DELAY_MS_SLOWEST;
    }

    if (pressedKeys->kpplus)
    {
        if (game->cellSize + CELL_SIZE_STEP <= MAX_CELL_SIZE) 
        {
            game->cellSize += CELL_SIZE_STEP;
            recalcCanvasDimensions = true;
        }
    }
    else if (pressedKeys->kpminus)
    {
        if (game->cellSize - CELL_SIZE_STEP >= MIN_CELL_SIZE) 
        {
            game->cellSize -= CELL_SIZE_STEP;
            recalcCanvasDimensions = true;
        }
    }

    if (pressedKeys->up_arrow)
    {
        CANVAS_OFFSET_Y += CANVAS_OFFSET_STEP;
        recalcCanvasDimensions = true;
    }
    else if (pressedKeys->down_arrow)
    {
        CANVAS_OFFSET_Y -= CANVAS_OFFSET_STEP;
        recalcCanvasDimensions = true;
    }
    else if (pressedKeys->left_arrow)
    {
        CANVAS_OFFSET_X += CANVAS_OFFSET_STEP;
        recalcCanvasDimensions = true;
    }
    else if (pressedKeys->right_arrow)
    {
        CANVAS_OFFSET_X -= CANVAS_OFFSET_STEP;
        recalcCanvasDimensions = true;
    }

    if (recalcCanvasDimensions)
    {
        gol_recalculate_canvas_dimensions(game);
    }

    if (game->shouldUpdate)
    {
        GOL_Update(game->numHorizontalGridLines, game->numVerticalGridLines, game->cells);
        SDL_Delay(DELAY_MS);
    }
}

void processEvent(SDL_Event *event)
{
    if (event->type == SDL_QUIT)
    {
        FORCE_QUIT = true;
    }

    if (event->type == SDL_KEYDOWN && event->key.state == SDL_PRESSED)
    {
        keysPressed.escape |= event->key.keysym.sym == SDLK_ESCAPE;
        keysPressed.space |= event->key.keysym.sym == SDLK_SPACE;
        keysPressed.r |= event->key.keysym.sym == SDLK_r;
        keysPressed.kp1 |= event->key.keysym.sym == SDLK_KP_1;
        keysPressed.kp2 |= event->key.keysym.sym == SDLK_KP_2;
        keysPressed.kp3 |= event->key.keysym.sym == SDLK_KP_3;
        keysPressed.kpplus |= event->key.keysym.sym == SDLK_KP_PLUS;
        keysPressed.kpminus |= event->key.keysym.sym == SDLK_KP_MINUS;
        keysPressed.up_arrow |= event->key.keysym.sym == SDLK_UP;
        keysPressed.down_arrow |= event->key.keysym.sym == SDLK_DOWN;
        keysPressed.left_arrow |= event->key.keysym.sym == SDLK_LEFT;
        keysPressed.right_arrow |= event->key.keysym.sym == SDLK_RIGHT;
    }

    if (event->type == SDL_MOUSEWHEEL)
    {
        if (event->wheel.y > 0)
        {
            // Scrolled up
            if (CELL_SIZE + CELL_SIZE_STEP <= MAX_CELL_SIZE) 
            {
                CELL_SIZE += CELL_SIZE_STEP;
                FLAG_WINDOW_DIMENSIONS_CHANGED = true;
            }
        }
        else if (event->wheel.y < 0)
        {
            // Scrolled down
            if (CELL_SIZE - CELL_SIZE_STEP >= MIN_CELL_SIZE) 
            {
                CELL_SIZE -= CELL_SIZE_STEP;
                FLAG_WINDOW_DIMENSIONS_CHANGED = true;
            }
        }
    }

    if (event->type == SDL_WINDOWEVENT)
    {
        if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            FLAG_WINDOW_DIMENSIONS_CHANGED = true;
        }
    }

    if (event->type == SDL_MOUSEBUTTONDOWN)
    {
        if (event->button.button == SDL_BUTTON_LEFT && event->button.state == SDL_PRESSED)
        {
            MOUSE_CLICK_POS_X = event->button.x;
            MOUSE_CLICK_POS_Y = event->button.y;
            FLAG_MOUSE_CLICKED = true;
        }
    }
}

void processSDLEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e) > 0)
    {
        processEvent(&e);
    }
}

int main()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window;
    SDL_Renderer *renderer;

    if (SDL_CreateWindowAndRenderer(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, 0, &window, &renderer))
    {
        printf("Failed to create window and renderer: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetWindowTitle(window, "Game of Life");
    SDL_SetWindowResizable(window, true);

    int sdlWindowHeight = 0;
    int sdlWindowWidth = 0;
    SDL_GetWindowSize(window, &sdlWindowWidth, &sdlWindowHeight);

    GameOfLife game;
    gol_init(&game, sdlWindowWidth, sdlWindowHeight);

    while (game.running && !FORCE_QUIT)
    {
        processSDLEvents(); //TODO(konnor): Possibly move this to end of loop to catch FORCE_QUIT faster

        if (FLAG_WINDOW_DIMENSIONS_CHANGED)
        {
            SDL_GetWindowSize(window, &sdlWindowWidth, &sdlWindowHeight);
            gol_window_resized(&game, sdlWindowWidth, sdlWindowHeight);
            FLAG_WINDOW_DIMENSIONS_CHANGED = false;
        }

        if (FLAG_MOUSE_CLICKED)
        {
            gol_mouse_clicked(&game);
            FLAG_MOUSE_CLICKED = false;
        }

        // Set background color
        SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
        SDL_RenderClear(renderer); // Clear rendering target with draw color
        gol_render(&game, renderer);
        SDL_RenderPresent(renderer);

        gol_update(&game, &keysPressed);
        keystates_reset(&keysPressed);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
