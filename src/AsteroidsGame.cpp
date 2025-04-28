// AsteroidsGame.cpp
#include "AsteroidsGame.h"

// --- Constructor ---
AsteroidsGame::AsteroidsGame(Adafruit_SSD1306 &disp) : display(disp) {
    currentState = START;
    score = 0;
    lives = 3;
    fireButtonPressedLastFrame = false;
    lastFireTime = 0;
    shipSpawnTime = 0;
    // IMPORTANT: Ensure randomSeed() is called in the main .ino setup()
}

// --- Public Methods ---

void AsteroidsGame::begin() {
    for (int i = 0; i < MAX_BULLETS; ++i) bullets[i].active = false;
    for (int i = 0; i < MAX_ASTEROIDS; ++i) asteroids[i].active = false;
    resetGame();
    currentState = START;
}

GameState AsteroidsGame::getCurrentState() {
    return currentState;
}

// Use fireButtonDown parameter, NOT digitalRead
void AsteroidsGame::update(int joyX, int joyY, bool fireButtonDown) {
    unsigned long currentTime = millis();

    switch (currentState) {
        case START:
            // Use passed button state and internal tracking for edge detection
            if (fireButtonDown && !fireButtonPressedLastFrame) {
                // Optional: Add debounce delay here if needed, but it blocks.
                // A non-blocking debounce timer would be better.
                // For simplicity now, we assume debounce handled externally or is minor.
                resetGame();
                currentState = GAME;
                // Prevent immediate re-trigger if button held:
                fireButtonPressedLastFrame = true;
                return; // Exit early to avoid processing rest of frame as GAME
            }
            break;

        case GAME:
            handleInput(joyX, joyY, fireButtonDown);
            updateGameObjects();
            handleCollisions(); // Asteroids potentially marked inactive here

            if (lives <= 0 && !ship.active) { // Check for game over FIRST
                currentState = GAME_OVER;
                fireButtonPressedLastFrame = true; // Prevent instant restart
                return; // Exit GAME state processing
            }
            // If not game over, THEN check for level clear
            else if (checkLevelClear() && lives > 0) {
                 // Level is clear! Call the function that handles the pause and spawning
                 spawnNewWave();
                 // Optional: Reset ship velocity/position for new wave?
                 // ship.vel.x = 0; ship.vel.y = 0;
                 // ship.pos.x = SCREEN_WIDTH / 2.0f; ship.pos.y = SCREEN_HEIGHT / 2.0f;
            }
            break; // End of case GAME

        case GAME_OVER:
            if (fireButtonDown && !fireButtonPressedLastFrame) {
                // Optional debounce
                currentState = START;
                // Game reset happens when transitioning START -> GAME
                // Prevent immediate re-trigger if button held:
                 fireButtonPressedLastFrame = true;
                 return;
            }
            break;
    }

    // Update tracked button state *at the end* for next frame's edge detection
    fireButtonPressedLastFrame = fireButtonDown;
}


void AsteroidsGame::draw() {
    display.clearDisplay();
    switch (currentState) {
        case START:
            drawStartMenu();
            break;
        case GAME:
            {
                bool invincible = (ship.lifetime > 0);
                if (ship.active) {
                    drawShip(invincible);
                }
                drawAsteroids();
                drawBullets();
                drawUI();
            }
            break;
        case GAME_OVER:
            drawGameOverScreen();
            break;
    }
    display.display();
}

// --- Private Method Implementations ---

void AsteroidsGame::resetGame() {
    score = 0;
    lives = 3;
    // Fix Vector2D assignment:
    ship.pos.x = SCREEN_WIDTH / 2.0f;
    ship.pos.y = SCREEN_HEIGHT / 2.0f;
    ship.vel.x = 0.0f;
    ship.vel.y = 0.0f;
    ship.angle = -M_PI / 2.0f;
    ship.radius = SHIP_COLLISION_RADIUS;
    ship.active = true;
    shipSpawnTime = millis();
    ship.lifetime = INVINCIBILITY_DURATION;

    for (int i = 0; i < MAX_BULLETS; ++i) bullets[i].active = false;
    for (int i = 0; i < MAX_ASTEROIDS; ++i) asteroids[i].active = false;

    for (int i = 0; i < STARTING_ASTEROIDS; ++i) {
        float spawnX, spawnY;
        do {
            spawnX = random(0, SCREEN_WIDTH);
            spawnY = random(0, SCREEN_HEIGHT);
        } while (sqrt(pow(spawnX - ship.pos.x, 2) + pow(spawnY - ship.pos.y, 2)) < ASTEROID_SIZE_LARGE * 2.5f);
        spawnAsteroid(ASTEROID_SIZE_LARGE, spawnX, spawnY);
    }
    fireButtonPressedLastFrame = true; // Prevent immediate action if button was held
}

// Use fireButtonDown parameter
void AsteroidsGame::handleInput(int joyX, int joyY, bool fireButtonDown) {
    if (!ship.active) {
        isThrusting = false; // Ensure thrust stops if ship inactive
        return;
    }

    // Rotation (X-Axis) with Dead Zone (Keep this logic)
    if (joyX < JOYSTICK_CENTER - JOYSTICK_DEAD_ZONE) {
        ship.angle -= SHIP_TURN_SPEED;
    } else if (joyX > JOYSTICK_CENTER + JOYSTICK_DEAD_ZONE) {
        ship.angle += SHIP_TURN_SPEED;
    }
    if (ship.angle < 0) ship.angle += 2 * M_PI;
    if (ship.angle >= 2 * M_PI) ship.angle -= 2 * M_PI;

    // Thrust (Y-Axis Forward) with Dead Zone
    // Reset thrust flag initially
    isThrusting = false;
    if (joyY < JOYSTICK_CENTER - JOYSTICK_DEAD_ZONE) { // Pushing Up/Forward
        // Apply acceleration in the direction the ship is facing
        ship.vel.x += cos(ship.angle) * SHIP_THRUST;
        ship.vel.y += sin(ship.angle) * SHIP_THRUST;
        isThrusting = true; // Set flag for drawing flame
    }
    if (joyY > JOYSTICK_CENTER + JOYSTICK_DEAD_ZONE) { // Pushing Up/Forward
        // Apply acceleration in the direction the ship is facing
        ship.vel.x -= cos(ship.angle) * SHIP_THRUST;
        ship.vel.y -= sin(ship.angle) * SHIP_THRUST;
        isThrusting = false; // Set flag for drawing flame
    }
    // No action needed for joyY > JOYSTICK_CENTER + JOYSTICK_DEAD_ZONE (pulling back)

    // Firing (Keep this logic, ensure bullet velocity includes ship velocity)
    unsigned long currentTime = millis();
    if (fireButtonDown && !fireButtonPressedLastFrame && (currentTime - lastFireTime > FIRE_DEBOUNCE_DELAY)) {
        int slot = findInactiveBulletSlot();
        if (slot != -1) {
            GameObject &newBullet = bullets[slot];
            float noseX = ship.pos.x + cos(ship.angle) * (ship.radius + 2);
            float noseY = ship.pos.y + sin(ship.angle) * (ship.radius + 2);

            newBullet.pos.x = noseX;
            newBullet.pos.y = noseY;
            // Bullet velocity includes ship's current velocity + bullet speed
            newBullet.vel.x = (cos(ship.angle) * BULLET_SPEED) + ship.vel.x;
            newBullet.vel.y = (sin(ship.angle) * BULLET_SPEED) + ship.vel.y;
            newBullet.angle = 0;
            newBullet.radius = BULLET_COLLISION_RADIUS;
            newBullet.active = true;
            newBullet.lifetime = BULLET_LIFETIME;
            newBullet.size = 0;

            lastFireTime = currentTime;
        }
    }
}

void AsteroidsGame::updateGameObjects() {
    unsigned long currentTime = millis();

    // Update Ship
    if (ship.active) {
        // Apply friction to existing velocity
        ship.vel.x *= SHIP_FRICTION;
        ship.vel.y *= SHIP_FRICTION;

        // Update position based on current velocity (which includes thrust input and friction)
        ship.pos.x += ship.vel.x;
        ship.pos.y += ship.vel.y;

        // Keep wrap around for the ship
        wrapAround(ship);

        // Keep invincibility timer logic
        if (currentTime - shipSpawnTime > INVINCIBILITY_DURATION) {
            ship.lifetime = 0;
        } else {
            ship.lifetime = INVINCIBILITY_DURATION - (currentTime - shipSpawnTime);
        }
    }

    // Update Bullets (Remains the same)
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets[i].active) {
            bullets[i].pos.x += bullets[i].vel.x;
            bullets[i].pos.y += bullets[i].vel.y;
            bullets[i].lifetime--;
            wrapAround(bullets[i]);
            if (bullets[i].lifetime <= 0) {
                bullets[i].active = false;
            }
        }
    }

    // Update Asteroids (Remains the same)
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].active) {
            asteroids[i].pos.x += asteroids[i].vel.x;
            asteroids[i].pos.y += asteroids[i].vel.y;
            wrapAround(asteroids[i]);
        }
    }
}

void AsteroidsGame::wrapAround(GameObject &obj) {
    // Use constants defined in .h
    if (obj.pos.x < -obj.radius) obj.pos.x = SCREEN_WIDTH + obj.radius;
    if (obj.pos.x > SCREEN_WIDTH + obj.radius) obj.pos.x = -obj.radius;
    if (obj.pos.y < -obj.radius) obj.pos.y = SCREEN_HEIGHT + obj.radius;
    if (obj.pos.y > SCREEN_HEIGHT + obj.radius) obj.pos.y = -obj.radius;
}


void AsteroidsGame::handleCollisions() {
    // Bullet-Asteroid
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets[i].active) continue;
        for (int j = 0; j < MAX_ASTEROIDS; ++j) {
            if (!asteroids[j].active) continue;
            float dx = bullets[i].pos.x - asteroids[j].pos.x;
            float dy = bullets[i].pos.y - asteroids[j].pos.y;
            float distanceSquared = dx * dx + dy * dy;
            float radiiSum = bullets[i].radius + asteroids[j].radius;
            if (distanceSquared < radiiSum * radiiSum) {
                bullets[i].active = false;
                asteroids[j].active = false;
                if (asteroids[j].size == ASTEROID_SIZE_LARGE) score += 20;
                else if (asteroids[j].size == ASTEROID_SIZE_MEDIUM) score += 50;
                else score += 100;
                if (asteroids[j].size == ASTEROID_SIZE_LARGE) {
                    spawnAsteroid(ASTEROID_SIZE_MEDIUM, asteroids[j].pos.x, asteroids[j].pos.y, asteroids[j].vel.x, asteroids[j].vel.y);
                    spawnAsteroid(ASTEROID_SIZE_MEDIUM, asteroids[j].pos.x, asteroids[j].pos.y, asteroids[j].vel.x, asteroids[j].vel.y);
                } else if (asteroids[j].size == ASTEROID_SIZE_MEDIUM) {
                    spawnAsteroid(ASTEROID_SIZE_SMALL, asteroids[j].pos.x, asteroids[j].pos.y, asteroids[j].vel.x, asteroids[j].vel.y);
                    spawnAsteroid(ASTEROID_SIZE_SMALL, asteroids[j].pos.x, asteroids[j].pos.y, asteroids[j].vel.x, asteroids[j].vel.y);
                }
                break;
            }
        }
    }
    // Ship-Asteroid
    bool currentlyInvincible = (ship.lifetime > 0);
    if (ship.active && !currentlyInvincible) {
        for (int j = 0; j < MAX_ASTEROIDS; ++j) {
            if (!asteroids[j].active) continue;
            float dx = ship.pos.x - asteroids[j].pos.x;
            float dy = ship.pos.y - asteroids[j].pos.y;
            float distanceSquared = dx * dx + dy * dy;
            float radiiSum = ship.radius + asteroids[j].radius;
            if (distanceSquared < radiiSum * radiiSum) {
                lives--;
                asteroids[j].active = false;
                if (lives > 0) {
                    // Fix Vector2D assignment:
                    ship.pos.x = SCREEN_WIDTH / 2.0f;
                    ship.pos.y = SCREEN_HEIGHT / 2.0f;
                    ship.vel.x = 0.0f;
                    ship.vel.y = 0.0f;
                    ship.angle = -M_PI / 2.0f;
                    shipSpawnTime = millis();
                    ship.lifetime = INVINCIBILITY_DURATION;
                } else {
                    ship.active = false;
                }
                break;
            }
        }
    }
}

void AsteroidsGame::drawShip(bool invincible) {
    if (invincible && (millis() / 200) % 2) return;
    float p1x = ship.radius + 2, p1y = 0;
    float p2x = -ship.radius,    p2y = -ship.radius + 1;
    float p3x = -ship.radius,    p3y = ship.radius - 1;
    rotatePoint(0, 0, ship.angle, p1x, p1y);
    rotatePoint(0, 0, ship.angle, p2x, p2y);
    rotatePoint(0, 0, ship.angle, p3x, p3y);
    p1x += ship.pos.x; p1y += ship.pos.y;
    p2x += ship.pos.x; p2y += ship.pos.y;
    p3x += ship.pos.x; p3y += ship.pos.y;
    display.drawTriangle(round(p1x), round(p1y), round(p2x), round(p2y), round(p3x), round(p3y), SSD1306_WHITE);
    // Draw thrust flame? Check internal state if possible, not hardware.
    // This simple check approximates thrust based on velocity:
    if (abs(ship.vel.x) > 0.1 || abs(ship.vel.y) > 0.1) { // Check if ship has notable velocity
         float flameX1 = -ship.radius;      float flameY1 = -ship.radius / 2;
         float flameX2 = -ship.radius - 3;  float flameY2 = 0;
         float flameX3 = -ship.radius;      float flameY3 = ship.radius / 2;
         rotatePoint(0, 0, ship.angle, flameX1, flameY1);
         rotatePoint(0, 0, ship.angle, flameX2, flameY2);
         rotatePoint(0, 0, ship.angle, flameX3, flameY3);
         display.drawTriangle(
             round(ship.pos.x + flameX1), round(ship.pos.y + flameY1),
             round(ship.pos.x + flameX2), round(ship.pos.y + flameY2),
             round(ship.pos.x + flameX3), round(ship.pos.y + flameY3),
             SSD1306_WHITE);
     }
}

void AsteroidsGame::drawAsteroids() {
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (!asteroids[i].active) continue;
        const GameObject &asteroid = asteroids[i];
        int numVertices = 5 + asteroid.size / 3;
        float angleStep = 2 * M_PI / numVertices;
        float lastX = 0, lastY = 0;
        float firstX = 0, firstY = 0;
        for (int v = 0; v < numVertices; ++v) {
            float angle = v * angleStep;
            float radius_variation = asteroid.radius * (random(70, 131) / 100.0f);
            float currentX = asteroid.pos.x + cos(angle) * radius_variation;
            float currentY = asteroid.pos.y + sin(angle) * radius_variation;
            if (v > 0) {
                display.drawLine(round(lastX), round(lastY), round(currentX), round(currentY), SSD1306_WHITE);
            } else {
                firstX = currentX;
                firstY = currentY;
            }
            lastX = currentX;
            lastY = currentY;
        }
        display.drawLine(round(lastX), round(lastY), round(firstX), round(firstY), SSD1306_WHITE);
    }
}

void AsteroidsGame::drawBullets() {
    display.setTextColor(SSD1306_WHITE);
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (bullets[i].active) {
            display.drawPixel(round(bullets[i].pos.x), round(bullets[i].pos.y), SSD1306_WHITE);
        }
    }
}

void AsteroidsGame::drawUI() {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(score);
    for (int i = 0; i < lives; ++i) {
        int iconX = SCREEN_WIDTH - (i * 9) - 8; // Use constant
        int iconY = 4;
        display.drawTriangle(iconX, iconY - 3, iconX - 3, iconY + 2, iconX + 3, iconY + 2, SSD1306_WHITE);
    }
}

void AsteroidsGame::drawStartMenu() {
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 10);
    display.print("ASTEROIDS");
    display.setTextSize(1);
    display.setCursor(18, 40);
    display.print("Press Fire Button");
    display.setCursor(35, 50);
    display.print("to Start");
}

void AsteroidsGame::drawGameOverScreen() {
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 10);
    display.print("GAME OVER");
    display.setTextSize(1);
    display.setCursor(35, 35);
    display.print("Score: ");
    display.print(score);
    display.setCursor(18, 50);
    display.print("Press Fire Button");
}

void AsteroidsGame::rotatePoint(float cx, float cy, float angle, float &x, float &y) {
    float tempX = x - cx;
    float tempY = y - cy;
    float rotatedX = tempX * cos(angle) - tempY * sin(angle);
    float rotatedY = tempX * sin(angle) + tempY * cos(angle);
    x = rotatedX + cx;
    y = rotatedY + cy;
}

bool AsteroidsGame::checkLevelClear() {
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (asteroids[i].active) {
            return false;
        }
    }
    return true;
}

void AsteroidsGame::spawnNewWave() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(30, SCREEN_HEIGHT / 2 - 4); // Use constant
    display.print("Wave Cleared!");
    display.display();
    delay(1500);

    int num_to_spawn = STARTING_ASTEROIDS + (score / 500);
    if (num_to_spawn > MAX_ASTEROIDS) num_to_spawn = MAX_ASTEROIDS;
    for (int i = 0; i < num_to_spawn; ++i) {
        float spawnX, spawnY;
        do {
            spawnX = random(0, SCREEN_WIDTH); // Use constant
            spawnY = random(0, SCREEN_HEIGHT); // Use constant
        } while (sqrt(pow(spawnX - ship.pos.x, 2) + pow(spawnY - ship.pos.y, 2)) < ASTEROID_SIZE_LARGE * 2.5f);
        spawnAsteroid(ASTEROID_SIZE_LARGE, spawnX, spawnY);
    }
}

// Ensure spawnAsteroid implementation is also correct
void AsteroidsGame::spawnAsteroid(int size, float x, float y, float initial_vx, float initial_vy) {
    int slot = findInactiveAsteroidSlot();
    if (slot == -1) return;
    GameObject &newAsteroid = asteroids[slot];

    if (x < 0 || y < 0) {
        if (random(0, 2) == 0) {
            newAsteroid.pos.x = random(0, SCREEN_WIDTH);
            newAsteroid.pos.y = (random(0, 2) == 0) ? 0 - size : SCREEN_HEIGHT + size;
        } else {
            newAsteroid.pos.x = (random(0, 2) == 0) ? 0 - size : SCREEN_WIDTH + size;
            newAsteroid.pos.y = random(0, SCREEN_HEIGHT);
        }
    } else {
        // Fix Vector2D assignment:
        newAsteroid.pos.x = x;
        newAsteroid.pos.y = y;
    }

    if (initial_vx == 0 && initial_vy == 0) {
        float speed = random(ASTEROID_SPEED_MIN * 100, ASTEROID_SPEED_MAX * 100) / 100.0f;
        float angle = random(0, 200 * M_PI) / 100.0f;
        // Fix Vector2D assignment:
        newAsteroid.vel.x = cos(angle) * speed;
        newAsteroid.vel.y = sin(angle) * speed;
    } else {
        float speed_variation = random(80, 120) / 100.0;
        float angle_variation = random(-20, 21) / 100.0;
        float current_angle = atan2(initial_vy, initial_vx);
        float current_speed = sqrt(initial_vx * initial_vx + initial_vy * initial_vy);
        // Fix Vector2D assignment:
        newAsteroid.vel.x = cos(current_angle + angle_variation) * current_speed * speed_variation;
        newAsteroid.vel.y = sin(current_angle + angle_variation) * current_speed * speed_variation;
        if(sqrt(newAsteroid.vel.x * newAsteroid.vel.x + newAsteroid.vel.y * newAsteroid.vel.y) < ASTEROID_SPEED_MIN * 0.8) {
             float speed = random(ASTEROID_SPEED_MIN * 100, ASTEROID_SPEED_MAX * 100) / 100.0f;
             float angle = random(0, 200 * M_PI) / 100.0f;
             newAsteroid.vel.x = cos(angle) * speed;
             newAsteroid.vel.y = sin(angle) * speed;
         }
    }
    newAsteroid.angle = 0;
    newAsteroid.radius = (float)size;
    newAsteroid.active = true;
    newAsteroid.lifetime = 0;
    newAsteroid.size = size;
}

int AsteroidsGame::findInactiveBulletSlot() {
    for (int i = 0; i < MAX_BULLETS; ++i) {
        if (!bullets[i].active) return i;
    }
    return -1;
}

int AsteroidsGame::findInactiveAsteroidSlot() {
    for (int i = 0; i < MAX_ASTEROIDS; ++i) {
        if (!asteroids[i].active) return i;
    }
    return -1;
}