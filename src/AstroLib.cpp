#include "AstroLib.h" // Includes GameData.h, AudioEngine.h indirectly via AstroLib.h

// --- Preferences Namespace ---
// Use a unique namespace for your application's preferences
// to avoid collisions with other libraries/sketches.
const char *PREFERENCES_NAMESPACE = "AstroLib";
const char *PREF_KEY_HIGH_SCORE = "highScore";

// --- Constructor ---
AstroLib::AstroLib(Adafruit_SSD1306 &disp) : display(disp), audio(), preferences(), // Initialize Preferences object
                                             fireButtonPin(-1), hyperspaceButtonPin(-1),
                                             currentState(START), score(0), lives(3), highScore(0), // Init highScore to 0 initially
                                             fireButtonPressedLastFrame(false), hyperspaceButtonPressedLastFrame(false),
                                             lastFireTime(0), shipSpawnTime(0), lastHyperspaceTime(0),
                                             isThrusting(false)
{
    ship.active = false;
}

// --- Public Method Implementations ---

void AstroLib::begin(int audioPin)
{
    // Initialize arrays
    for (int i = 0; i < MAX_BULLETS; ++i)
        bullets[i].active = false;
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
        asteroids[i].active = false;

    // Initialize Preferences - MUST be done before accessing NVS
    // The 'false' parameter indicates read/write mode.
    preferences.begin(PREFERENCES_NAMESPACE, false);

    // Load the high score from NVS AFTER beginning preferences
    loadHighScore();

    resetGame(); // Sets up initial game state (doesn't reset loaded high score)
    currentState = START;

    audio.begin(audioPin);

    Serial.print("Astrolib Initialized. Loaded High Score: ");
    Serial.println(highScore);
}

// --- Configuration ---
void AstroLib::attachFireButtonPin(int pin)
{ // Renamed
    fireButtonPin = pin;
    if (fireButtonPin >= 0)
    {
        pinMode(fireButtonPin, INPUT_PULLUP);
    }
}

void AstroLib::attachHyperspaceButtonPin(int pin)
{ // Renamed
    hyperspaceButtonPin = pin;
    if (hyperspaceButtonPin >= 0)
    {
        pinMode(hyperspaceButtonPin, INPUT_PULLUP);
    }
}

GameState AstroLib::getCurrentState() { return currentState; }
int AstroLib::getScore() { return score; }
int AstroLib::getHighScore() { return highScore; }
void AstroLib::resetHighScore()
{
    highScore = 0;
    saveHighScore();
}

void AstroLib::update(int joyX, int joyY, bool joyButtonDown) {
    // --- Read Digital Buttons ---
    bool digitalFireDown = (fireButtonPin >= 0) && (digitalRead(fireButtonPin) == LOW);
    bool digitalHyperspaceDown = (hyperspaceButtonPin >= 0) && (digitalRead(hyperspaceButtonPin) == LOW);
    bool anyFireButtonDown = joyButtonDown || digitalFireDown;

    // --- State Transitions & Logic ---
    switch (currentState) {
        // ... (case START remains the same) ...
         case START:
            if (anyFireButtonDown && !fireButtonPressedLastFrame) {
                resetGame();
                currentState = GAME;
                fireButtonPressedLastFrame = true;
                hyperspaceButtonPressedLastFrame = true;
                return;
            }
            break;

        case GAME:
            handleInput(joyX, joyY, anyFireButtonDown, digitalHyperspaceDown);
            updateGameObjects();
            handleCollisions();

            if (lives <= 0 && !ship.active) {
                // --- Game Over Transition ---
                currentState = GAME_OVER;
                // Update & Save High Score if needed
                if (score > highScore) {
                    Serial.print("New High Score! "); Serial.println(score);
                    highScore = score;
                    saveHighScore(); // <<< SAVE TO NVS
                }
                fireButtonPressedLastFrame = true;
                hyperspaceButtonPressedLastFrame = true;
                audio.stopAllSounds();
                return;
            }
            // ... (checkLevelClear remains the same) ...
            else if (checkLevelClear() && lives > 0) {
                 spawnNewWave();
            }
            break;

        // ... (case GAME_OVER remains the same) ...
         case GAME_OVER:
             if (anyFireButtonDown && !fireButtonPressedLastFrame) {
                 currentState = START;
                 fireButtonPressedLastFrame = true;
                 hyperspaceButtonPressedLastFrame = true;
                 return;
             }
             break;
    }

    audio.update();
    fireButtonPressedLastFrame = anyFireButtonDown;
    hyperspaceButtonPressedLastFrame = digitalHyperspaceDown;
}

void AstroLib::draw()
{ // Renamed
    display.clearDisplay();
    switch (currentState)
    {
    case START:
        drawStartMenu();
        break;
    case GAME:
    {
        bool invincible = (ship.lifetime > 0);
        if (ship.active)
        {
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

// --- Core Logic ---
void AstroLib::resetGame()
{
    score = 0;
    lives = 3;
    ship.pos.x = SCREEN_WIDTH / 2.0f;
    ship.pos.y = SCREEN_HEIGHT / 2.0f;
    ship.vel.x = 0.0f;
    ship.vel.y = 0.0f;
    ship.angle = -M_PI / 2.0f;
    ship.radius = SHIP_COLLISION_RADIUS;
    ship.active = true;
    shipSpawnTime = millis();
    ship.lifetime = INVINCIBILITY_DURATION;
    for (int i = 0; i < MAX_BULLETS; ++i)
        bullets[i].active = false;
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
        asteroids[i].active = false;
    for (int i = 0; i < STARTING_ASTEROIDS; ++i)
    {
        float spawnX, spawnY;
        do
        {
            spawnX = random(0, SCREEN_WIDTH);
            spawnY = random(0, SCREEN_HEIGHT);
        } while (sqrt(pow(spawnX - ship.pos.x, 2) + pow(spawnY - ship.pos.y, 2)) < ASTEROID_SIZE_LARGE * 2.5f);
        spawnAsteroid(ASTEROID_SIZE_LARGE, spawnX, spawnY);
    }
    fireButtonPressedLastFrame = true;
    hyperspaceButtonPressedLastFrame = true;
    lastHyperspaceTime = millis() - HYPERSPACE_COOLDOWN;
    isThrusting = false;
    audio.stopAllSounds();
}

void AstroLib::handleInput(int joyX, int joyY, bool anyFireButtonDown, bool digitalHyperspaceDown)
{
    if (!ship.active)
    {
        if (isThrusting)
        {
            audio.stopThrustSound();
            isThrusting = false;
        }
        return;
    }

    // --- Rotation (Variable Speed) ---
    int xDelta = joyX - JOYSTICK_CENTER;
    float turnScale = 0.0f;
    if (abs(xDelta) > JOYSTICK_DEAD_ZONE)
    {
        turnScale = (float)(abs(xDelta) - JOYSTICK_DEAD_ZONE) / (JOYSTICK_MAX_THROW - JOYSTICK_DEAD_ZONE);
        turnScale = max(0.0f, min(1.0f, turnScale));
        if (xDelta < 0)
        {
            ship.angle -= SHIP_TURN_SPEED * turnScale;
        }
        else
        {
            ship.angle += SHIP_TURN_SPEED * turnScale;
        }
        if (ship.angle < 0)
            ship.angle += 2 * M_PI;
        if (ship.angle >= 2 * M_PI)
            ship.angle -= 2 * M_PI;
    }

    // --- Thrust (Variable Speed & Sound) ---
    int yDelta = joyY - JOYSTICK_CENTER;
    bool wantsToThrust = (yDelta < -JOYSTICK_DEAD_ZONE);
    float thrustScale = 0.0f;
    if (wantsToThrust)
    {
        thrustScale = (float)(abs(yDelta) - JOYSTICK_DEAD_ZONE) / (JOYSTICK_MAX_THROW - JOYSTICK_DEAD_ZONE);
        thrustScale = max(0.0f, min(1.0f, thrustScale));
        ship.vel.x += cos(ship.angle) * SHIP_THRUST * thrustScale;
        ship.vel.y += sin(ship.angle) * SHIP_THRUST * thrustScale;
    }
    if (wantsToThrust && !isThrusting)
    {
        audio.startThrustSound(thrustScale);
    }
    else if (!wantsToThrust && isThrusting)
    {
        audio.stopThrustSound();
    }
    // else if (wantsToThrust && isThrusting) { /* Optional: update pitch */ }
    isThrusting = wantsToThrust;

    // --- Firing ---
    unsigned long currentTime = millis();
    if (anyFireButtonDown && !fireButtonPressedLastFrame && (currentTime - lastFireTime > FIRE_DEBOUNCE_DELAY))
    {
        int slot = findInactiveBulletSlot();
        if (slot != -1)
        {
            GameObject &newBullet = bullets[slot];
            float noseX = ship.pos.x + cos(ship.angle) * (ship.radius + 2);
            float noseY = ship.pos.y + sin(ship.angle) * (ship.radius + 2);
            newBullet.pos.x = noseX;
            newBullet.pos.y = noseY;
            newBullet.vel.x = (cos(ship.angle) * BULLET_SPEED) + ship.vel.x;
            newBullet.vel.y = (sin(ship.angle) * BULLET_SPEED) + ship.vel.y;
            newBullet.angle = 0;
            newBullet.radius = BULLET_COLLISION_RADIUS;
            newBullet.active = true;
            newBullet.lifetime = BULLET_LIFETIME;
            newBullet.size = 0;
            lastFireTime = currentTime;
            audio.playShootSound();
        }
    }

    // --- Hyperspace ---
    if (digitalHyperspaceDown && !hyperspaceButtonPressedLastFrame)
    {
        if (currentTime - lastHyperspaceTime > HYPERSPACE_COOLDOWN)
        {
            triggerHyperspace();
            lastHyperspaceTime = currentTime;
        }
    }
}

void AstroLib::triggerHyperspace()
{
    if (!ship.active)
        return;
    audio.playHyperspaceSound();
    ship.pos.x = random(ship.radius * 2, SCREEN_WIDTH - ship.radius * 2);
    ship.pos.y = random(ship.radius * 2, SCREEN_HEIGHT - ship.radius * 2);
    ship.vel.x = 0.0f;
    ship.vel.y = 0.0f;
    shipSpawnTime = millis();
    ship.lifetime = HYPERSPACE_INVINCIBILITY;
    if (isThrusting)
    {
        audio.stopThrustSound();
        isThrusting = false;
    }
}

void AstroLib::loadHighScore() {
    // Load high score from NVS.
    // The second argument to getInt is the default value if the key doesn't exist.
    highScore = preferences.getInt(PREF_KEY_HIGH_SCORE, 0);
    Serial.print("Loaded HS from NVS: "); Serial.println(highScore); // Debug
}

void AstroLib::saveHighScore() {
    // Save the current highScore variable to NVS
    preferences.putInt(PREF_KEY_HIGH_SCORE, highScore);
    Serial.print("Saved HS to NVS: "); Serial.println(highScore); // Debug
    // Note: preferences.end() could be called if done saving, but keeping it open
    // is fine if you might save other things later (like settings).
    // If you call end(), you need to call begin() again before the next load/save.
}

void AstroLib::updateGameObjects()
{
    unsigned long currentTime = millis();
    if (ship.active)
    {
        ship.vel.x *= SHIP_FRICTION;
        ship.vel.y *= SHIP_FRICTION;
        ship.pos.x += ship.vel.x;
        ship.pos.y += ship.vel.y;
        wrapAround(ship);
        if (currentTime - shipSpawnTime > INVINCIBILITY_DURATION)
        {
            ship.lifetime = 0;
        }
        else
        {
            ship.lifetime = INVINCIBILITY_DURATION - (currentTime - shipSpawnTime);
        }
    }
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        if (bullets[i].active)
        {
            bullets[i].pos.x += bullets[i].vel.x;
            bullets[i].pos.y += bullets[i].vel.y;
            bullets[i].lifetime--;
            wrapAround(bullets[i]);
            if (bullets[i].lifetime <= 0)
            {
                bullets[i].active = false;
            }
        }
    }
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        if (asteroids[i].active)
        {
            asteroids[i].pos.x += asteroids[i].vel.x;
            asteroids[i].pos.y += asteroids[i].vel.y;
            wrapAround(asteroids[i]);
        }
    }
}

void AstroLib::handleCollisions()
{
    // --- Bullet-Asteroid Collisions ---
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        if (!bullets[i].active)
            continue;
        for (int j = 0; j < MAX_ASTEROIDS; ++j)
        {
            if (!asteroids[j].active)
                continue;

            float dx = bullets[i].pos.x - asteroids[j].pos.x;
            float dy = bullets[i].pos.y - asteroids[j].pos.y;
            float distanceSquared = dx * dx + dy * dy;
            float radiiSum = bullets[i].radius + asteroids[j].radius;

            if (distanceSquared < radiiSum * radiiSum)
            {
                bullets[i].active = false;
                asteroids[j].active = false;
                audio.playExplosionSound(); // Play explosion sound

                // Award score
                if (asteroids[j].size == ASTEROID_SIZE_LARGE)
                    score += 20;
                else if (asteroids[j].size == ASTEROID_SIZE_MEDIUM)
                    score += 50;
                else
                    score += 100;

                // Break asteroid
                if (asteroids[j].size == ASTEROID_SIZE_LARGE)
                {
                    spawnAsteroid(ASTEROID_SIZE_MEDIUM, asteroids[j].pos.x, asteroids[j].pos.y, asteroids[j].vel.x, asteroids[j].vel.y);
                    spawnAsteroid(ASTEROID_SIZE_MEDIUM, asteroids[j].pos.x, asteroids[j].pos.y, asteroids[j].vel.x, asteroids[j].vel.y);
                }
                else if (asteroids[j].size == ASTEROID_SIZE_MEDIUM)
                {
                    spawnAsteroid(ASTEROID_SIZE_SMALL, asteroids[j].pos.x, asteroids[j].pos.y, asteroids[j].vel.x, asteroids[j].vel.y);
                    spawnAsteroid(ASTEROID_SIZE_SMALL, asteroids[j].pos.x, asteroids[j].pos.y, asteroids[j].vel.x, asteroids[j].vel.y);
                }
                break; // Bullet hits only one asteroid
            }
        }
    }

    // --- Ship-Asteroid Collisions ---
    bool currentlyInvincible = (ship.lifetime > 0);
    if (ship.active && !currentlyInvincible)
    {
        for (int j = 0; j < MAX_ASTEROIDS; ++j)
        {
            if (!asteroids[j].active)
                continue;

            float dx = ship.pos.x - asteroids[j].pos.x;
            float dy = ship.pos.y - asteroids[j].pos.y;
            float distanceSquared = dx * dx + dy * dy;
            float radiiSum = ship.radius + asteroids[j].radius;

            if (distanceSquared < radiiSum * radiiSum)
            {
                lives--;
                asteroids[j].active = false; // Destroy asteroid on collision
                audio.playExplosionSound();  // Play explosion sound

                if (lives > 0)
                {
                    // Respawn: Reset position, velocity, grant invincibility
                    ship.pos.x = SCREEN_WIDTH / 2.0f;
                    ship.pos.y = SCREEN_HEIGHT / 2.0f;
                    ship.vel.x = 0.0f;
                    ship.vel.y = 0.0f;
                    ship.angle = -M_PI / 2.0f;
                    shipSpawnTime = millis();
                    ship.lifetime = INVINCIBILITY_DURATION;
                }
                else
                {
                    ship.active = false; // Game Over (state change handled in update())
                }
                break; // Ship hit one asteroid this frame
            }
        }
    }
}

bool AstroLib::checkLevelClear()
{
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        if (asteroids[i].active)
        {
            return false; // Found an active asteroid
        }
    }
    return true; // No active asteroids found
}

void AstroLib::spawnNewWave()
{
    audio.stopAllSounds();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(30, SCREEN_HEIGHT / 2 - 4);
    display.print("Wave Cleared!");
    display.display();
    delay(1500);
    int num_to_spawn = STARTING_ASTEROIDS + (score / 500);
    if (num_to_spawn > MAX_ASTEROIDS)
        num_to_spawn = MAX_ASTEROIDS;
    for (int i = 0; i < num_to_spawn; ++i)
    {
        float spawnX, spawnY;
        do
        {
            spawnX = random(0, SCREEN_WIDTH);
            spawnY = random(0, SCREEN_HEIGHT);
        } while (sqrt(pow(spawnX - ship.pos.x, 2) + pow(spawnY - ship.pos.y, 2)) < ASTEROID_SIZE_LARGE * 3.0f);
        spawnAsteroid(ASTEROID_SIZE_LARGE, spawnX, spawnY);
    }
}

// --- Object Management ---

void AstroLib::spawnAsteroid(int size, float x, float y, float initial_vx, float initial_vy)
{
    int slot = findInactiveAsteroidSlot();
    if (slot == -1)
        return; // No space left
    GameObject &newAsteroid = asteroids[slot];

    // Determine spawn position (edge or specified point)
    if (x < 0 || y < 0)
    { // Spawn at edge if no position specified
        if (random(0, 2) == 0)
        { // Top/Bottom or Left/Right edge
            newAsteroid.pos.x = random(0, SCREEN_WIDTH);
            newAsteroid.pos.y = (random(0, 2) == 0) ? 0 - size : SCREEN_HEIGHT + size;
        }
        else
        {
            newAsteroid.pos.x = (random(0, 2) == 0) ? 0 - size : SCREEN_WIDTH + size;
            newAsteroid.pos.y = random(0, SCREEN_HEIGHT);
        }
    }
    else
    { // Spawn at specified position (for fragments)
        newAsteroid.pos.x = x;
        newAsteroid.pos.y = y;
    }

    // Determine velocity (random or based on parent)
    if (initial_vx == 0 && initial_vy == 0)
    { // New asteroid
        float speed = random(ASTEROID_SPEED_MIN * 100, ASTEROID_SPEED_MAX * 100) / 100.0f;
        float angle = random(0, 200 * M_PI) / 100.0f; // 0 to 2*PI
        newAsteroid.vel.x = cos(angle) * speed;
        newAsteroid.vel.y = sin(angle) * speed;
    }
    else
    {                                                    // Fragment - inherit velocity with variation
        float speed_variation = random(80, 120) / 100.0; // 0.8x to 1.2x speed
        float angle_variation = random(-25, 26) / 100.0; // +/- 0.25 radians (~14 deg)
        float parent_angle = atan2(initial_vy, initial_vx);
        float parent_speed = sqrt(initial_vx * initial_vx + initial_vy * initial_vy);
        float new_speed = parent_speed * speed_variation;
        // Ensure minimum speed for fragments
        if (new_speed < ASTEROID_SPEED_MIN)
            new_speed = ASTEROID_SPEED_MIN;

        newAsteroid.vel.x = cos(parent_angle + angle_variation) * new_speed;
        newAsteroid.vel.y = sin(parent_angle + angle_variation) * new_speed;
    }

    // Set remaining properties
    newAsteroid.angle = 0; // Asteroids don't visually rotate here
    newAsteroid.radius = (float)size;
    newAsteroid.active = true;
    newAsteroid.lifetime = 0; // Not used
    newAsteroid.size = size;
}

int AstroLib::findInactiveBulletSlot()
{
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        if (!bullets[i].active)
            return i;
    }
    return -1; // No available slot
}

int AstroLib::findInactiveAsteroidSlot()
{
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        if (!asteroids[i].active)
            return i;
    }
    return -1; // No available slot
}

void AstroLib::wrapAround(GameObject &obj)
{
    if (obj.pos.x < -obj.radius)
        obj.pos.x = SCREEN_WIDTH + obj.radius;
    else if (obj.pos.x > SCREEN_WIDTH + obj.radius)
        obj.pos.x = -obj.radius;

    if (obj.pos.y < -obj.radius)
        obj.pos.y = SCREEN_HEIGHT + obj.radius;
    else if (obj.pos.y > SCREEN_HEIGHT + obj.radius)
        obj.pos.y = -obj.radius;
}

// --- Drawing ---

void AstroLib::drawShip(bool invincible)
{
    // Blink if invincible
    if (invincible && (millis() / 200) % 2)
        return;

    // Ship vertices relative to (0,0)
    float p1x = ship.radius + 2, p1y = 0;             // Nose
    float p2x = -ship.radius, p2y = -ship.radius + 1; // Back left
    float p3x = -ship.radius, p3y = ship.radius - 1;  // Back right

    // Rotate points
    rotatePoint(0, 0, ship.angle, p1x, p1y);
    rotatePoint(0, 0, ship.angle, p2x, p2y);
    rotatePoint(0, 0, ship.angle, p3x, p3y);

    // Translate to ship position
    p1x += ship.pos.x;
    p1y += ship.pos.y;
    p2x += ship.pos.x;
    p2y += ship.pos.y;
    p3x += ship.pos.x;
    p3y += ship.pos.y;

    // Draw ship triangle
    display.drawTriangle(round(p1x), round(p1y), round(p2x), round(p2y), round(p3x), round(p3y), SSD1306_WHITE);

    // Draw thrust flame if thrusting
    if (isThrusting)
    {
        float flameX1 = -ship.radius;
        float flameY1 = -ship.radius / 2;
        float flameX2 = -ship.radius - 3;
        float flameY2 = 0;
        float flameX3 = -ship.radius;
        float flameY3 = ship.radius / 2;
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

void AstroLib::drawAsteroids()
{
    for (int i = 0; i < MAX_ASTEROIDS; ++i)
    {
        if (!asteroids[i].active)
            continue;
        const GameObject &asteroid = asteroids[i];

        // Draw jagged polygon
        int numVertices = 5 + asteroid.size / 3; // Vary vertices with size
        float angleStep = 2 * M_PI / numVertices;
        float lastX = 0, lastY = 0, firstX = 0, firstY = 0;

        for (int v = 0; v < numVertices; ++v)
        {
            float angle = v * angleStep;
            float radius_variation = asteroid.radius * (random(70, 131) / 100.0f); // Jaggedness
            float currentX = asteroid.pos.x + cos(angle) * radius_variation;
            float currentY = asteroid.pos.y + sin(angle) * radius_variation;

            if (v > 0)
            {
                display.drawLine(round(lastX), round(lastY), round(currentX), round(currentY), SSD1306_WHITE);
            }
            else
            {
                firstX = currentX;
                firstY = currentY; // Store first point
            }
            lastX = currentX;
            lastY = currentY;
        }
        // Close the polygon
        display.drawLine(round(lastX), round(lastY), round(firstX), round(firstY), SSD1306_WHITE);
    }
}

void AstroLib::drawBullets()
{
    // display.setTextColor(SSD1306_WHITE); // Not needed for pixels
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        if (bullets[i].active)
        {
            display.drawPixel(round(bullets[i].pos.x), round(bullets[i].pos.y), SSD1306_WHITE);
        }
    }
}

void AstroLib::drawUI()
{
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Draw Score (Top Left)
    display.setCursor(1, 1);
    display.print(score);

    // Draw High Score (Top Right) - Simple approach
    String hsText = "HI:"; // Using String for ease, char array is more efficient
    hsText += highScore;
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(hsText, 0, 0, &x1, &y1, &w, &h); // Measure text width
    display.setCursor(SCREEN_WIDTH - w - 1, 1);            // Position from right
    display.print(hsText);

    // Draw Lives (Bottom Left - moved from top right)
    for (int i = 0; i < lives; ++i)
    {
        int iconX = 2 + (i * 9);       // Position from left
        int iconY = SCREEN_HEIGHT - 6; // Position from bottom
        display.drawTriangle(iconX, iconY - 3, iconX - 3, iconY + 2, iconX + 3, iconY + 2, SSD1306_WHITE);
    }
}

void AstroLib::drawStartMenu()
{
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(15, 10);
    display.print("ASTEROIDS"); // Using AstroLib name would require text width calculation
    display.setTextSize(1);
    display.setCursor(18, 40);
    display.print("Press Fire Button");
    display.setCursor(35, 50);
    display.print("to Start");
}

void AstroLib::drawGameOverScreen()
{
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 10);
    display.print("GAME OVER");

    display.setTextSize(1);
    display.setCursor(25, 35);
    display.print("Score: ");
    display.print(score);

    display.setCursor(25, 45); // Add High Score display
    display.print("High:  ");
    display.print(highScore);

    display.setCursor(18, 55);
    display.print("Press Fire Button");
}

// --- Utility ---

void AstroLib::rotatePoint(float cx, float cy, float angle, float &x, float &y)
{
    float tempX = x - cx;
    float tempY = y - cy;
    float rotatedX = tempX * cos(angle) - tempY * sin(angle);
    float rotatedY = tempX * sin(angle) + tempY * cos(angle);
    x = rotatedX + cx;
    y = rotatedY + cy;
}