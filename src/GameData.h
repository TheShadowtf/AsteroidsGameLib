// GameData.h
#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <Arduino.h>

// --- Screen Dimensions ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// --- Game States ---
enum GameState { START, GAME, GAME_OVER };

// --- Game Object Structures ---
struct Vector2D {
    float x;
    float y;
};

struct GameObject {
    Vector2D pos;
    Vector2D vel;
    float angle;
    float radius;
    bool active; // Ensure this definition is correct
    int lifetime;
    int size;
};

// --- Input Constants ---
const int JOYSTICK_CENTER = 2048;
const int JOYSTICK_DEAD_ZONE = 400;
const int JOYSTICK_MAX_THROW = 2047; // Max deviation from center (4095 - 2048 approx)

// --- Game Tuning Constants ---
// ... (Ship Turn/Thrust/Friction, Bullet Speed/Lifetime/Max) ...
const float SHIP_TURN_SPEED = 0.12;  // Max turn speed
const float SHIP_THRUST = 0.20;      // Max thrust acceleration
const float SHIP_FRICTION = 0.97;
const float BULLET_SPEED = 3.5;
const int   BULLET_LIFETIME = 40;
const int   MAX_BULLETS = 5;
// ... (Asteroid Speed/Max/Starting/Sizes) ...
const float ASTEROID_SPEED_MIN = 0.5;
const float ASTEROID_SPEED_MAX = 1.5;
const int   MAX_ASTEROIDS = 10;
const int   STARTING_ASTEROIDS = 3;
const float SHIP_COLLISION_RADIUS = 4.0;
const float BULLET_COLLISION_RADIUS = 1.0;
const int   ASTEROID_SIZE_LARGE = 10;
const int   ASTEROID_SIZE_MEDIUM = 6;
const int   ASTEROID_SIZE_SMALL = 3;
// ... (Invincibility, Fire Debounce) ...
const unsigned long INVINCIBILITY_DURATION = 2000;
const unsigned long FIRE_DEBOUNCE_DELAY = 150;
// --- NEW ---
const unsigned long HYPERSPACE_COOLDOWN = 5000; // 5 seconds between jumps
const unsigned long HYPERSPACE_INVINCIBILITY = 750; // Shorter invincibility after jump


// --- Audio Frequencies (Hz) & Durations (ms) ---
const uint16_t SND_SHOOT_FREQ = 2500;
const uint16_t SND_EXPLODE_FREQ = 300;
const uint16_t SND_THRUST_FREQ_LOW = 100; // NEW: Range for variable thrust sound
const uint16_t SND_THRUST_FREQ_HIGH = 250;
const uint16_t SND_HYPERSPACE_FREQ = 4000; // NEW
const uint32_t SND_SHORT_DURATION = 50;
const uint32_t SND_HYPERSPACE_DURATION = 300; // NEW

#endif // GAME_DATA_H