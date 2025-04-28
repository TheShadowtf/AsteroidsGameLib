# ESP32 OLED Asteroids Game Library 🚀

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A simple, vector-graphics-inspired Asteroids clone game implemented as an Arduino library for ESP32 microcontrollers, using an SSD1306 OLED display and an analog joystick.

## Overview

This library encapsulates the core game logic for a classic Asteroids game, making it easy to integrate into your ESP32 projects. It handles:

*   Ship movement (rotation and forward thrust with inertia/friction)
*   Asteroid spawning, movement, screen wrapping, and breaking into smaller pieces
*   Bullet firing and lifetime management
*   Collision detection (ship-asteroid, bullet-asteroid)
*   Scoring, lives, and wave progression
*   Basic game states (Start Menu, Game, Game Over)
*   Rendering game objects with a vector-like style (lines/outlines) to an SSD1306 display.

## Features ✨

*   Classic Asteroids gameplay mechanics.
*   Vector-style graphics using `Adafruit_GFX`.
*   State machine for game flow (Start, Play, Game Over).
*   Screen wrapping for all game objects.
*   Asteroids break into smaller fragments upon destruction.
*   Basic scoring and lives system.
*   Wave progression with increasing difficulty (more asteroids).
*   Configurable constants for game tuning (speeds, counts, etc.).
*   Designed as an Arduino library for easy reuse.

## Hardware Required ⚙️

*   **ESP32 Development Board** (tested on various common modules)
*   **SSD1306 OLED Display (128x64, I2C)**
*   **Analog Joystick Module** (2-axis with push button switch)
*   Breadboard and Jumper Wires

## Dependencies 🔗

This library requires the following Arduino libraries to be installed:

1.  **Adafruit GFX Library:** [https://github.com/adafruit/Adafruit-GFX-Library](https://github.com/adafruit/Adafruit-GFX-Library)
2.  **Adafruit SSD1306 Library:** [https://github.com/adafruit/Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306)

You can install these using the Arduino Library Manager (`Sketch` -> `Include Library` -> `Manage Libraries...`).

## Installation

1.  **Download:** Click the "Code" button on this repository page and select "Download ZIP".
2.  **Install in Arduino IDE:** Open the Arduino IDE, navigate to `Sketch` -> `Include Library` -> `Add .ZIP Library...`, and select the downloaded ZIP file.
3.  **Restart IDE:** Restart the Arduino IDE to ensure the library is recognized.

## Hardware Setup / Wiring 🔌

Connect the components to your ESP32 as follows (adjust pins in the example sketch if necessary, but these are the defaults used):

| Component        | Pin Name | ESP32 Pin          | Notes                             |
| ---------------- | -------- | ------------------ | --------------------------------- |
| OLED Display     | VCC      | **3.3V**           | Connect to 3.3V, NOT 5V or Vin!   |
| OLED Display     | GND      | GND                | Common Ground                     |
| OLED Display     | SDA      | GPIO 21 (Default)  | I2C Data                          |
| OLED Display     | SCL      | GPIO 22 (Default)  | I2C Clock                         |
| Analog Joystick  | VCC / +  | **3.3V**           | Connect to 3.3V for correct range |
| Analog Joystick  | GND      | GND                | Common Ground                     |
| Analog Joystick  | VRx      | **GPIO 34**        | X-axis Analog Input               |
| Analog Joystick  | VRy      | **GPIO 35**        | Y-axis Analog Input               |
| Analog Joystick  | SW       | **GPIO 33**        | Button Digital Input              |

**Important:** Ensure both the OLED and Joystick are powered from the ESP32's **3.3V** output for correct voltage levels, especially for the analog joystick readings.

## Usage (Example)

After installing the library and setting up the hardware, you can use it in your sketch like this:

```cpp
// BasicAsteroids.ino (Example Sketch)

#include <Wire.h>
#include <Adafruit_GFX.h>     // Dependency
#include <Adafruit_SSD1306.h> // Dependency
#include <AsteroidsGame.h>    // Your library!

// --- Display Setup ---
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Uses constants from library

// --- Pin Definitions ---
const int VRx_PIN = 34;
const int VRy_PIN = 35;
const int FIRE_BUTTON_PIN = 33;

// --- Game Object ---
AsteroidsGame game(display); // Create an instance of the game

// --- Setup ---
void setup() {
    Serial.begin(115200);

    // Initialize Display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }

    // Initialize Input Pins
    pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);

    // Seed Random Generator
    randomSeed(millis()); // Or use analogRead(A0) if available

    // Initialize the Game Logic via the library
    game.begin();

    Serial.println("Asteroids Example Started");
}

// --- Main Loop ---
void loop() {
    unsigned long frameStart = millis();

    // 1. Read Inputs
    int joyXValue = analogRead(VRx_PIN);
    int joyYValue = analogRead(VRy_PIN);
    bool fireButtonState = (digitalRead(FIRE_BUTTON_PIN) == LOW);

    // 2. Update Game State (pass inputs to the library)
    game.update(joyXValue, joyYValue, fireButtonState);

    // 3. Draw Game Screen (library handles all drawing)
    game.draw();

    // 4. Frame Limiting
    unsigned long frameTime = millis() - frameStart;
    if (frameTime < 33) { // Aim for ~30 FPS
        delay(33 - frameTime);
    }
}
