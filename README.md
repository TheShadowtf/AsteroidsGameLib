# Modular ESP32 Game Framework - Asteroids Example 🚀

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**A highly adaptable game development library for ESP32, demonstrated via a classic Asteroids clone.** This project aims to provide a flexible framework for creating simple 2D games on ESP32 using various displays, input methods, and optional audio.

The current core provides a functional Asteroids game using an SSD1306 OLED display and an analog joystick.

![Gameplay Screenshot/GIF]
_([ YOUR GIF/SCREENSHOT HERE ] - Replace this line with an actual image/gif link once uploaded, e.g., `![Gameplay Screenshot](docs/gameplay.gif)` )_

## Vision & Goals ✨

The goal is to evolve this library into a mini-framework where core game logic is separated from hardware specifics. Key design goals include:

*   **Hardware Abstraction:** Easily swap display drivers (OLED, LCD), input controllers (buttons, joysticks, potentiometers), and audio outputs.
*   **Modularity:** Enable/disable features like audio or specific game elements easily.
*   **Extensibility:** Provide a base for creating other simple 2D arcade-style games.
*   **Performance:** Keep the core loop efficient for ESP32 capabilities.

## Current Features (Asteroids Implementation)

*   ✅ Classic Asteroids gameplay mechanics (ship rotation/thrust, asteroids, bullets).
*   ✅ Vector-style graphics rendering (lines/outlines) via `Adafruit_GFX`.
*   ✅ Specific support for SSD1306 128x64 OLED display (I2C).
*   ✅ Specific support for 2-axis analog joystick with button input.
*   ✅ Basic state machine (Start Menu, Game, Game Over).
*   ✅ Screen wrapping for game objects.
*   ✅ Asteroids break into smaller fragments.
*   ✅ Basic scoring and lives system.
*   ✅ Wave progression (simple difficulty increase).

## Roadmap 🗺️

This outlines the planned development path. Checked items are implemented in the current version.

**Core Gameplay Engine:**
*   [x] Basic Entity Management (Ship, Asteroids, Bullets)
*   [x] 2D Physics (Position, Velocity, Angle, Friction)
*   [x] Collision Detection (Simple Circle-based)
*   [x] Game State Machine

**Display Abstraction:**
*   [ ] Abstract Display Driver Interface (Define common drawing methods)
*   [x] Concrete SSD1306 I2C Driver Implementation
*   [ ] Add support for other OLED displays (e.g., SH1106)
*   [ ] Add support for common LCD displays (e.g., ST7735, ST7789, ILI9341 via SPI)
*   [ ] Support for different screen resolutions/orientations

**Input Abstraction:**
*   [ ] Abstract Input Controller Interface (Define common actions: Up, Down, Left, Right, Fire, Select, Back, Hyperspace)
*   [x] Concrete Analog Joystick Implementation (now with variable intensity)
*   [x] Add support for Digital Buttons (GPIO) mapping to Fire action
*   [x] Add support for Digital Buttons (GPIO) mapping to Hyperspace action
*   [ ] Support for other analog inputs (e.g., potentiometers)

**Audio Engine:**
*   [ ] Abstract Audio Output Interface (Play tone, Play SFX)
*   [x] Add support for basic Tone output (ESP32 `tone()`)
*   [ ] Add support for simple DAC/PWM audio playback (requires more setup)
*   [x] Implement basic sound effects (Shoot, Explosion, Thrust, Hyperspace)
*   [x] Variable pitch for thrust sound (basic implementation)
*   [ ] Add optional background music support (simple looping tones)

**User Interface:**
*   [x] Basic Start/Game Over Screens
*   [ ] Implement a navigable Menu System (using abstracted input)
*   [x] Add High Score display / tracking (in memory)
*   [ ] Add High Score saving/loading (e.g., NVS/Preferences)
*   [ ] Add basic Settings screen (e.g., toggle sound, adjust difficulty?)

**Gameplay Enhancements (Asteroids Specific):**
*   [ ] Add UFO Enemy (different movement patterns/firing)
*   [x] Implement Hyperspace jump feature (with cooldown & risk - risk currently just random placement)
*   [ ] Refine difficulty scaling (more sophisticated than just asteroid count)

**Technical Improvements:**
*   [ ] Replace `delay()` calls with non-blocking timers (e.g., for wave transitions, debounce)
*   [ ] Code optimization for performance/memory usage.
*   [ ] Improved code documentation (inline comments, Doxygen?).
*   [x] Split library code into multiple files (`.h`/`.cpp`) for better organization.
*   [ ] Add more examples demonstrating different features/hardware.

## Hardware Required (Current Example) ⚙️

*   **ESP32 Development Board**
*   **SSD1306 OLED Display (128x64, I2C)**
*   **Analog Joystick Module** (2-axis with push button switch)
*   Breadboard and Jumper Wires

*(Note: Future versions aim to support a wider range of hardware)*

## Dependencies 🔗

Requires the following libraries (install via Arduino Library Manager):

1.  **Adafruit GFX Library**
2.  **Adafruit SSD1306**

*(Note: Dependencies may change as display/audio support is added)*

## Installation

1.  **Download:** Click "Code" -> "Download ZIP".
2.  **Install:** In Arduino IDE: `Sketch` -> `Include Library` -> `Add .ZIP Library...`.
3.  **Restart IDE.**

## Hardware Setup / Wiring (Current Example) 🔌

Connect components to your ESP32 as follows (Default pins for the basic example):

| Component        | Pin Name | ESP32 Pin          | Notes                             |
| ---------------- | -------- | ------------------ | --------------------------------- |
| OLED Display     | VCC      | **3.3V**           | **Use 3.3V!**                     |
| OLED Display     | GND      | GND                |                                   |
| OLED Display     | SDA      | GPIO 21 (Default)  |                                   |
| OLED Display     | SCL      | GPIO 22 (Default)  |                                   |
| Analog Joystick  | VCC / +  | **3.3V**           | **Use 3.3V!**                     |
| Analog Joystick  | GND      | GND                |                                   |
| Analog Joystick  | VRx      | **GPIO 34**        |                                   |
| Analog Joystick  | VRy      | **GPIO 35**        |                                   |
| Analog Joystick  | SW       | **GPIO 33**        |                                   |

*(Note: Pin assignments can be changed in the example sketch but require code modification until input abstraction is complete)*

## Basic Usage (Current Example)

```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AsteroidsGame.h> // Include the library

// --- Display Setup ---
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Uses constants from library

// --- Pin Definitions ---
const int VRx_PIN = 34;
const int VRy_PIN = 35;
const int FIRE_BUTTON_PIN = 33;

// --- Game Object ---
// Pass the specific display object for now
AsteroidsGame game(display);

// --- Setup ---
void setup() {
    Serial.begin(115200);
    // Initialize Display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { /* Error handling */ }
    // Initialize Input Pins
    pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);
    // Seed Random Generator
    randomSeed(millis());
    // Initialize the Game Logic
    game.begin();
    Serial.println("Asteroids Example Started");
}

// --- Main Loop ---
void loop() {
    unsigned long frameStart = millis();
    // 1. Read Inputs (specific to current hardware)
    int joyXValue = analogRead(VRx_PIN);
    int joyYValue = analogRead(VRy_PIN);
    bool fireButtonState = (digitalRead(FIRE_BUTTON_PIN) == LOW);
    // 2. Update Game State (pass raw/processed inputs)
    game.update(joyXValue, joyYValue, fireButtonState);
    // 3. Draw Game Screen
    game.draw();
    // 4. Frame Limiting
    unsigned long frameTime = millis() - frameStart;
    if (frameTime < 33) { delay(33 - frameTime); }
}
