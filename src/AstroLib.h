#ifndef ASTRO_LIB_H
#define ASTRO_LIB_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <cmath>
#include <Preferences.h>
#include "GameData.h"       // Include shared data definitions FIRST
#include "AudioEngine.h"    // Include the audio engine

class AstroLib { // Renamed class
public:
    AstroLib(Adafruit_SSD1306 &display); // Renamed constructor

    // --- Configuration ---
    void attachFireButtonPin(int pin);
    void attachHyperspaceButtonPin(int pin);

    // --- Core Methods ---
    void begin(int audioPin);
    void update(int joyX, int joyY, bool joyButtonDown);
    void draw();
    GameState getCurrentState();
    int getScore();
    int getHighScore();
    void resetHighScore();

private:
    // Dependencies
    Adafruit_SSD1306 &display;
    AudioEngine audio;
    Preferences preferences;

    // Hardware Pins
    int fireButtonPin;
    int hyperspaceButtonPin;

    // Game State
    GameState currentState;
    GameObject ship; // These use the definition from GameData.h
    GameObject bullets[MAX_BULLETS];
    GameObject asteroids[MAX_ASTEROIDS];
    int score;
    int lives;
    int highScore;

    // Input & Timing State
    bool fireButtonPressedLastFrame;
    bool hyperspaceButtonPressedLastFrame;
    unsigned long lastFireTime;
    unsigned long shipSpawnTime;
    unsigned long lastHyperspaceTime;
    bool isThrusting;

    // --- Private Helper Methods ---
    // Core Logic
    void resetGame();
    // UPDATED DECLARATION TO MATCH DEFINITION
    void handleInput(int joyX, int joyY, bool anyFireButtonDown, bool digitalHyperspaceDown);
    void updateGameObjects();
    void handleCollisions();
    bool checkLevelClear();
    void spawnNewWave();
    void triggerHyperspace();

    // Object Management & Drawing
    void spawnAsteroid(int size, float x = -1, float y = -1, float initial_vx = 0, float initial_vy = 0);
    int findInactiveBulletSlot();
    int findInactiveAsteroidSlot();
    void wrapAround(GameObject &obj);
    void drawShip(bool invincible);
    void drawAsteroids();
    void drawBullets();
    void drawUI();
    void drawStartMenu();
    void drawGameOverScreen();

    // Utility
    void rotatePoint(float cx, float cy, float angle, float &x, float &y);

    // NVS Helpers
    void loadHighScore();
    void saveHighScore();

};

#endif // ASTRO_LIB_H