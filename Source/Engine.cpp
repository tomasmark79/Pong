#include "Engine/Engine.hpp"

#include <engine/version.h>
#include <iostream>

#include "raylib.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <filesystem>
#include <random>
#include <thread>
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

typedef struct AudioSample
{
    Sound sound;
    bool  loaded;
} AudioSample;

std::vector<AudioSample> notes;

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

static int score = 0;

//------------------------------------------------------------------------------------
// Module Functions Declaration (local)
//------------------------------------------------------------------------------------
static void InitGame(void);        // Initialize game
static void UpdateGame(void);      // Update game (one frame)
static void DrawGame(void);        // Draw game (one frame)
static void UnloadGame(void);      // Unload game
static void UpdateDrawFrame(void); // Update and Draw (one frame)
static void InitNotes(std::vector<AudioSample> &notes);
static void PlayRandomNote();
static void PlayCDur();
static void PlayProgressionCDur();
static void PlayProgressionCMinor();
static void PlayProgressionCMinorReversed();
static void PlayRandomNoteInCMinorProgression();

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

    // get current working directory modern c++
    std::filesystem::path path = std::filesystem::current_path();
    std::cout << "Current path is : " << path << std::endl;

    InitAudioDevice();

    AudioSample c1, cis1, d1, dis1, e1, f1, fis1, g1, gis1, a1, ais1, b1, c2, cis2, d2, dis2, e2,
        f2, fis2, g2, gis2, a2, ais2, b2, c3, cis3, d3, dis3, e3, f3, fis3, g3, gis3, a3, ais3, b3,
        c4, cis4, d4, dis4, e4, f4, fis4, g4, gis4, a4, ais4, b4;

    notes = {c1,   cis1, d1, dis1, e1, f1,   fis1, g1, gis1, a1, ais1, b1, c2,   cis2, d2, dis2, e2,
             f2,   fis2, g2, gis2, a2, ais2, b2,   c3, cis3, d3, dis3, e3, f3,   fis3, g3, gis3, a3,
             ais3, b3,   c4, cis4, d4, dis4, e4,   f4, fis4, g4, gis4, a4, ais4, b4};

    InitNotes(notes);

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
    PlayProgressionCDur();

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
                ball.position = (Vector2){player.position.x + (ball.radius * 2), player.position.y};
            }

            // Collision logic: ball vs walls
            if ((ball.position.x + ball.radius) >= screenWidth)
            {
                ball.speed.x *= -1;
                PlayRandomNoteInCMinorProgression();
            }

            if (ball.active && (ball.position.x - ball.radius) <= 0)
            {
                ball.speed = (Vector2){0, 0};
                PlayRandomNoteInCMinorProgression();
                ball.active = false;
                player.life--;
                // PlayProgressionCMinor();
                PlayProgressionCMinorReversed();
            }

            if (((ball.position.y - ball.radius) <= 0 ||
                 ((ball.position.y + ball.radius) >= screenHeight)))
            {
                ball.speed.y *= -1;
                PlayRandomNoteInCMinorProgression();
            }

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
                    PlayRandomNoteInCMinorProgression();
                    score++;
                }
            }

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

        std::string scoreStr = "Score:\t" + std::to_string(score);
        DrawText(scoreStr.c_str(), 10, 10, 20, MAROON);

        // Draw player lives
        for (int i = 0; i < player.life; i++)
            DrawText("*", screenWidth - 100 - (40 * i), screenHeight - 40, 40, MAROON);

        // Draw ball
        DrawCircleV(ball.position, ball.radius, MAROON);

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

void InitNotes(std::vector<AudioSample> &notes)
{
    notes[0].sound = LoadSound("../Assets/C1.wav");
    notes[1].sound = LoadSound("../Assets/C#1.wav");
    notes[2].sound = LoadSound("../Assets/D1.wav");
    notes[3].sound = LoadSound("../Assets/D#1.wav");
    notes[4].sound = LoadSound("../Assets/E1.wav");
    notes[5].sound = LoadSound("../Assets/F1.wav");
    notes[6].sound = LoadSound("../Assets/F#1.wav");
    notes[7].sound = LoadSound("../Assets/G1.wav");
    notes[8].sound = LoadSound("../Assets/G#1.wav");
    notes[9].sound = LoadSound("../Assets/A1.wav");
    notes[10].sound = LoadSound("../Assets/A#1.wav");
    notes[11].sound = LoadSound("../Assets/B1.wav");

    notes[12].sound = LoadSound("../Assets/C2.wav");
    notes[13].sound = LoadSound("../Assets/C#2.wav");
    notes[14].sound = LoadSound("../Assets/D2.wav");
    notes[15].sound = LoadSound("../Assets/D#2.wav");
    notes[16].sound = LoadSound("../Assets/E2.wav");
    notes[17].sound = LoadSound("../Assets/F2.wav");
    notes[18].sound = LoadSound("../Assets/F#2.wav");
    notes[19].sound = LoadSound("../Assets/G2.wav");
    notes[20].sound = LoadSound("../Assets/G#2.wav");
    notes[21].sound = LoadSound("../Assets/A2.wav");
    notes[22].sound = LoadSound("../Assets/A#2.wav");
    notes[23].sound = LoadSound("../Assets/B2.wav");

    notes[24].sound = LoadSound("../Assets/C3.wav");
    notes[25].sound = LoadSound("../Assets/C#3.wav");
    notes[26].sound = LoadSound("../Assets/D3.wav");
    notes[27].sound = LoadSound("../Assets/D#3.wav");
    notes[28].sound = LoadSound("../Assets/E3.wav");
    notes[29].sound = LoadSound("../Assets/F3.wav");
    notes[30].sound = LoadSound("../Assets/F#3.wav");
    notes[31].sound = LoadSound("../Assets/G3.wav");
    notes[32].sound = LoadSound("../Assets/G#3.wav");
    notes[33].sound = LoadSound("../Assets/A3.wav");
    notes[34].sound = LoadSound("../Assets/A#3.wav");
    notes[35].sound = LoadSound("../Assets/B3.wav");

    notes[36].sound = LoadSound("../Assets/C4.wav");
    notes[37].sound = LoadSound("../Assets/C#4.wav");
    notes[38].sound = LoadSound("../Assets/D4.wav");
    notes[39].sound = LoadSound("../Assets/D#4.wav");
    notes[40].sound = LoadSound("../Assets/E4.wav");
    notes[41].sound = LoadSound("../Assets/F4.wav");
    notes[42].sound = LoadSound("../Assets/F#4.wav");
    notes[43].sound = LoadSound("../Assets/G4.wav");
    notes[44].sound = LoadSound("../Assets/G#4.wav");
    notes[45].sound = LoadSound("../Assets/A4.wav");
    notes[46].sound = LoadSound("../Assets/A#4.wav");
    notes[47].sound = LoadSound("../Assets/B4.wav");
}

void PlayRandomNote()
{
    // modern c++ random number generation
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, 47); // 48 notes
    int                             randomNote = dis(gen);
    PlaySound(notes[randomNote].sound);
}

void PlayRandomNoteInCMinorProgression()
{
    std::vector<int> cMinorProgression = {
        0, 3, 7, 10, 12, 15, 19, 22, 24, 27, 31, 34, 36, 39, 43, 46
    };
    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(0, 15); // 16 notes
    int                             randomNote = dis(gen);
    PlaySound(notes[cMinorProgression[randomNote]].sound);
}

void PlayCDur()
{
    PlaySound(notes[0].sound);
    PlaySound(notes[4].sound);
    PlaySound(notes[7].sound);
}

void PlayProgressionCDur()
{
    PlaySound(notes[0].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[4].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[7].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[12].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[16].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[19].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[24].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[28].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[31].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[36].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[40].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[43].sound);
}

void PlayProgressionCMinor()
{
    PlaySound(notes[0].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[3].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[7].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[10].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[12].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[15].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[19].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[22].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[24].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[27].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[31].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[34].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[36].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[39].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[43].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[46].sound);
}

void PlayProgressionCMinorReversed()
{
    PlaySound(notes[46].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[43].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[39].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[36].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[34].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[31].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[27].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[24].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[22].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[19].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[15].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[12].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[10].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[7].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[3].sound);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PlaySound(notes[0].sound);
}