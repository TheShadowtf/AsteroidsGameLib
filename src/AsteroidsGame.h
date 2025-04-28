// AsteroidsGame.h
#ifndef ASTEROIDS_GAME_H
#define ASTEROIDS_GAME_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <cmath>

// --- Screen Dimensions (Needed by the library) ---
#define SCREEN_WIDTH 128 // Define screen size here
#define SCREEN_HEIGHT 64

// --- Input Constants ---
const int JOYSTICK_CENTER = 1800; // Approx center for ESP32 ADC (0-4095)
const int JOYSTICK_DEAD_ZONE = 600; // Ignore inputs +/- this value around center (adjust if needed)

// --- Game Constants (Define ALL needed constants here) ---
const float SHIP_TURN_SPEED = 0.1;
const float SHIP_THRUST = 0.15;
// const float SHIP_VERTICAL_SPEED = 1.5;
const float SHIP_FRICTION = 0.98;
const float BULLET_SPEED = 3.0; // Ensure this is here
const int   BULLET_LIFETIME = 40;
const int   MAX_BULLETS = 5;
const float ASTEROID_SPEED_MIN = 0.5;
const float ASTEROID_SPEED_MAX = 1.5;
const int   MAX_ASTEROIDS = 10;
const int   STARTING_ASTEROIDS = 1;
const float SHIP_COLLISION_RADIUS = 4.0;
const float BULLET_COLLISION_RADIUS = 1.0;
const int   ASTEROID_SIZE_LARGE = 10;
const int   ASTEROID_SIZE_MEDIUM = 6;
const int   ASTEROID_SIZE_SMALL = 3;
const unsigned long INVINCIBILITY_DURATION = 2000;
const unsigned long FIRE_DEBOUNCE_DELAY = 200;

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
    bool active;
    int lifetime;
    int size;
};

class AsteroidsGame {
public:
    AsteroidsGame(Adafruit_SSD1306 &display);
    void begin();
    // Pass button state directly, library doesn't need to know the PIN
    void update(int joyX, int joyY, bool fireButtonDown);
    void draw();
    GameState getCurrentState();

private:
    Adafruit_SSD1306 &display;
    GameState currentState;
    GameObject ship;
    GameObject bullets[MAX_BULLETS];
    GameObject asteroids[MAX_ASTEROIDS];
    int score;
    int lives;
    bool fireButtonPressedLastFrame;
    bool isThrusting;
    unsigned long lastFireTime;
    unsigned long shipSpawnTime;

    void resetGame();
    void spawnAsteroid(int size, float x = -1, float y = -1, float initial_vx = 0, float initial_vy = 0);
    int findInactiveBulletSlot();
    int findInactiveAsteroidSlot();
    // Pass button state directly
    void handleInput(int joyX, int joyY, bool fireButtonDown);
    void updateGameObjects();
    void wrapAround(GameObject &obj);
    void handleCollisions();
    void drawShip(bool invincible);
    void drawAsteroids();
    void drawBullets();
    void drawUI();
    void drawStartMenu();
    void drawGameOverScreen();
    void rotatePoint(float cx, float cy, float angle, float &x, float &y);
    bool checkLevelClear();
    void spawnNewWave();
};

#endif // ASTEROIDS_GAME_H