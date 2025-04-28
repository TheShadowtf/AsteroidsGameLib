// Your main sketch file (e.g., ESP32_Asteroids.ino)

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "AsteroidsGame.h" // Include your new library

// --- Display Setup ---
// #define SCREEN_WIDTH 128 // No longer needed here, defined in library .h
// #define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Use library constants

// --- Pin Definitions ---
const int VRx_PIN = 34;
const int VRy_PIN = 35;
const int FIRE_BUTTON_PIN = 33;

// --- Game Object ---
AsteroidsGame game(display);

// --- Setup ---
void setup() {
    Serial.begin(115200);

    // Initialize Display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    display.clearDisplay();
    display.display();

    // Initialize Input Pins
    pinMode(FIRE_BUTTON_PIN, INPUT_PULLUP);

    // Seed Random Generator (IMPORTANT for library)
    // Try an unconnected analog pin like A0 (GPIO 36 if available) or another floating pin
    // If A0/36 isn't available or gives poor results, you might use a different pin or millis()
    // randomSeed(analogRead(A0)); // Or use a known floating pin for your ESP32 board
    randomSeed(millis());      // Fallback if analog noise isn't great

    // Initialize the Game Logic via the library
    game.begin();

    Serial.println("Setup Complete. Starting Game...");
}

// --- Main Loop ---
void loop() {
    unsigned long frameStart = millis();

    // 1. Read Inputs
    int joyXValue = analogRead(VRx_PIN);
    int joyYValue = analogRead(VRy_PIN);
    bool fireButtonState = (digitalRead(FIRE_BUTTON_PIN) == LOW); // Read physical button

    Serial.print("X: ");
    Serial.print(joyXValue);
    Serial.print(" | Y: ");
    Serial.print(joyYValue);
    Serial.print(" | Button: ");
    Serial.println(fireButtonState ? "DOWN" : "UP");

    // 2. Update Game State (pass inputs to the library)
    game.update(joyXValue, joyYValue, fireButtonState); // Pass the read state

    // 3. Draw Game Screen
    game.draw();

    // 4. Frame Limiting
    unsigned long frameTime = millis() - frameStart;
    if (frameTime < 33) { // Aim for ~30 FPS
        delay(33 - frameTime);
    }
}