#include "Engine/Engine.hpp"

// #include <EmojiTools/EmojiTools.hpp> // yet dissabled
#include <engine/version.h>
#include <iostream>

#include "raylib.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <vector>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

//----------------------------------------------------------------------------------
// Some Defines
//----------------------------------------------------------------------------------
#define PLAYER_MAX_LIFE 5
#define LINES_OF_BRICKS 5
#define BRICKS_PER_LINE 20

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef struct Player
{
    Vector2 position;
    Vector2 size;
    int     life;
} Player;

typedef struct Ball
{
    Vector2 position;
    Vector2 speed;
    int     radius;
    bool    active;
} Ball;

typedef struct Brick
{
    Vector2 position;
    bool    active;
} Brick;

//------------------------------------------------------------------------------------
// Global Variables Declaration
//------------------------------------------------------------------------------------
static const int screenWidth = 800;
static const int screenHeight = 600;

static bool gameOver = false;
static bool pause = false;

static Player  player = {0};
static Ball    ball = {0};
static Brick   brick[LINES_OF_BRICKS][BRICKS_PER_LINE] = {0};
static Vector2 brickSize = {0};

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);        // Initialize game
static void UpdateGame(void);      // Update game (one frame)
static void DrawGame(void);        // Draw game (one frame)
static void UnloadGame(void);      // Unload game
static void UpdateDrawFrame(void); // Update and Draw (one frame)

Engine::Engine()
{
    std::cout << "--- Engine v." << ENGINE_VERSION << " instantiated ---" << std::endl;

    // Initialization (Note windowTitle is unused on Android)
    //---------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "classic game: pong");

    Vector2 mainMonitorPosition = GetMonitorPosition(0);
    SetWindowPosition(
        mainMonitorPosition.x + (GetMonitorWidth(0) - GetScreenWidth()) / 2,
        mainMonitorPosition.y + (GetMonitorHeight(0) - GetScreenHeight()) / 2
    );

    // InitGame();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(120);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update and Draw
        //----------------------------------------------------------------------------------
        UpdateDrawFrame();
        //----------------------------------------------------------------------------------
    }
#endif
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadGame(); // Unload loaded data (textures, sounds, models...)

    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
}

Engine::~Engine()
{
    std::cout << "--- Engine uninstantiated ---" << std::endl;
}

//------------------------------------------------------------------------------------
// Module Functions Definitions (local)
//------------------------------------------------------------------------------------

// Initialize game variables
void InitGame(void)
{
    // brickSize = (Vector2){(float)GetScreenWidth() / BRICKS_PER_LINE, 40.0f};

    // Initialize player
    player.position = (Vector2){57, screenHeight / 2};
    player.size = (Vector2){14, screenHeight / 6};
    player.life = PLAYER_MAX_LIFE;

    // Initialize ball
    ball.position = (Vector2){player.position.x + ball.radius,
                              player.position.y - player.size.y / 2 - ball.radius};


    ball.speed = (Vector2){0, 0};
    ball.radius = 7;
    ball.active = false;
}

// Update game (one frame)
void UpdateGame(void)
{
    if (!gameOver)
    {
        if (IsKeyPressed('P'))
            pause = !pause;

        if (!pause)
        {
            // Player movement logic
            if (IsKeyDown(KEY_UP))
                player.position.y -= 5;
            if ((player.position.y - (player.size.y / 2)) <= 0)
                player.position.y = (player.size.y / 2);

            if (IsKeyDown(KEY_DOWN))
                player.position.y += 5;
            if ((player.position.y + player.size.y / 2) >= screenHeight)
                player.position.y = screenHeight - (player.size.y / 2);

            // Ball launching logic
            if (!ball.active)
            {
                if (IsKeyPressed(KEY_SPACE))
                {
                    ball.active = true;
                    ball.speed = (Vector2){5, 0};
                }
            }

            // Ball movement logic
            if (ball.active)
            {
                ball.position.x += ball.speed.x;
                ball.position.y += ball.speed.y;
            }
            else
            {
                ball.position = (Vector2){player.position.x + (ball.radius * 2),
                                          player.position.y};
            }

            // Collision logic: ball vs walls
            if ((ball.position.x + ball.radius) >= screenWidth)
                ball.speed.x *= -1;

            if ((ball.position.x - ball.radius) <= 0)
            {
                ball.speed = (Vector2){0, 0};
            }

            if (((ball.position.y - ball.radius) <= 0 ||
                 ((ball.position.y + ball.radius) >= screenHeight)))
                ball.speed.y *= -1;

            // if ((ball.position.y + ball.radius) >= screenHeight)
            //{
            //  ball.speed = (Vector2){0, 0};
            //  ball.active = false;

            // player.life--;
            //}

            // Collision logic: ball vs player
            if (CheckCollisionCircleRec(
                    ball.position,
                    ball.radius,
                    (Rectangle){player.position.x - player.size.x / 2,
                                player.position.y - player.size.y / 2,
                                player.size.x,
                                player.size.y}
                ))
            {
                if (ball.speed.x < 0)
                {
                    ball.speed.x *= -1;
                    ball.speed.y = (ball.position.y - player.position.y) / (player.size.y / 2) * 5;
                }
            }

            // // Collision logic: ball vs bricks
            // for (int i = 0; i < LINES_OF_BRICKS; i++)
            // {
            //     for (int j = 0; j < BRICKS_PER_LINE; j++)
            //     {
            //         if (brick[i][j].active)
            //         {
            //             // Hit below
            //             if (((ball.position.y - ball.radius) <=
            //                  (brick[i][j].position.y + brickSize.y / 2)) &&
            //                 ((ball.position.y - ball.radius) >
            //                  (brick[i][j].position.y + brickSize.y / 2 + ball.speed.y)) &&
            //                 ((fabs(ball.position.x - brick[i][j].position.x)) <
            //                  (brickSize.x / 2 + ball.radius * 2 / 3)) &&
            //                 (ball.speed.y < 0))
            //             {
            //                 brick[i][j].active = false;
            //                 ball.speed.y *= -1;
            //             }
            //             // Hit above
            //             else if (((ball.position.y + ball.radius) >=
            //                       (brick[i][j].position.y - brickSize.y / 2)) &&
            //                      ((ball.position.y + ball.radius) <
            //                       (brick[i][j].position.y - brickSize.y / 2 + ball.speed.y)) &&
            //                      ((fabs(ball.position.x - brick[i][j].position.x)) <
            //                       (brickSize.x / 2 + ball.radius * 2 / 3)) &&
            //                      (ball.speed.y > 0))
            //             {
            //                 brick[i][j].active = false;
            //                 ball.speed.y *= -1;
            //             }
            //             // Hit left
            //             else if (((ball.position.x + ball.radius) >=
            //                       (brick[i][j].position.x - brickSize.x / 2)) &&
            //                      ((ball.position.x + ball.radius) <
            //                       (brick[i][j].position.x - brickSize.x / 2 + ball.speed.x)) &&
            //                      ((fabs(ball.position.y - brick[i][j].position.y)) <
            //                       (brickSize.y / 2 + ball.radius * 2 / 3)) &&
            //                      (ball.speed.x > 0))
            //             {
            //                 brick[i][j].active = false;
            //                 ball.speed.x *= -1;
            //             }
            //             // Hit right
            //             else if (((ball.position.x - ball.radius) <=
            //                       (brick[i][j].position.x + brickSize.x / 2)) &&
            //                      ((ball.position.x - ball.radius) >
            //                       (brick[i][j].position.x + brickSize.x / 2 + ball.speed.x)) &&
            //                      ((fabs(ball.position.y - brick[i][j].position.y)) <
            //                       (brickSize.y / 2 + ball.radius * 2 / 3)) &&
            //                      (ball.speed.x < 0))
            //             {
            //                 brick[i][j].active = false;
            //                 ball.speed.x *= -1;
            //             }
            //         }
            //     }
            // }

            // Game over logic
            if (player.life <= 0)
                gameOver = true;
            else
            {
                // gameOver = true;

                // for (int i = 0; i < LINES_OF_BRICKS; i++)
                // {
                //     for (int j = 0; j < BRICKS_PER_LINE; j++)
                //     {
                //         if (brick[i][j].active)
                //             // gameOver = false;
                //     }
                // }
            }
        }
    }
    else
    {
        if (IsKeyPressed(KEY_ENTER))
        {
            InitGame();
            gameOver = false;
        }
    }
}

// Draw game (one frame)
void DrawGame(void)
{
    BeginDrawing();

    ClearBackground(RAYWHITE);

    if (!gameOver)
    {
        // Draw player bar with unified coordinates
        DrawRectangle(
            player.position.x - player.size.x / 2,
            player.position.y - player.size.y / 2,
            player.size.x,
            player.size.y,
            BLACK
        );

        // Draw player lives
        for (int i = 0; i < player.life; i++)
            DrawText("*", screenWidth - 100 - (40 * i), screenHeight - 40, 40, MAROON);

        // Draw ball
        DrawCircleV(ball.position, ball.radius, MAROON);

        // // Draw bricks
        // for (int i = 0; i < LINES_OF_BRICKS; i++)
        // {
        //     for (int j = 0; j < BRICKS_PER_LINE; j++)
        //     {
        //         if (brick[i][j].active)
        //         {
        //             if ((i + j) % 2 == 0)
        //                 DrawRectangle(
        //                     brick[i][j].position.x - brickSize.x / 2,
        //                     brick[i][j].position.y - brickSize.y / 2,
        //                     brickSize.x,
        //                     brickSize.y,
        //                     GRAY
        //                 );
        //             else
        //                 DrawRectangle(
        //                     brick[i][j].position.x - brickSize.x / 2,
        //                     brick[i][j].position.y - brickSize.y / 2,
        //                     brickSize.x,
        //                     brickSize.y,
        //                     DARKGRAY
        //                 );
        //         }
        //     }
        // }

        if (pause)
            DrawText(
                "GAME PAUSED",
                screenWidth / 2 - MeasureText("GAME PAUSED", 40) / 2,
                screenHeight / 2 - 40,
                40,
                GRAY
            );
    }
    else
        DrawText(
            "PRESS [ENTER] TO PLAY AGAIN",
            GetScreenWidth() / 2 - MeasureText("PRESS [ENTER] TO PLAY AGAIN", 20) / 2,
            GetScreenHeight() / 2 - 50,
            20,
            GRAY
        );

    EndDrawing();
}

// Unload game variables
void UnloadGame(void)
{
    // TODO: Unload all dynamic loaded data (textures, sounds, models...)
}

// Update and Draw (one frame)
void UpdateDrawFrame(void)
{
    UpdateGame();
    DrawGame();
}