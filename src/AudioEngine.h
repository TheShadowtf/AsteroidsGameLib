#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <Arduino.h>
#include "GameData.h"

class AudioEngine {
public:
    AudioEngine();
    void begin(uint8_t pin);
    void update();

    void playShootSound();
    void playExplosionSound();
    void playHyperspaceSound(); // NEW
    // Optionally pass thrust intensity (0.0 to 1.0) for pitch variation
    void startThrustSound(float intensity = 1.0f);
    void stopThrustSound();
    void stopAllSounds();

private:
    uint8_t buzzerPin;
    bool initialized;
    bool thrustSoundActive;
    uint16_t currentContinuousFreq;
    unsigned long soundEndTime;

    // duration 0 = continuous, intensity for thrust pitch
    void playTone(uint16_t freq, uint32_t duration = 0);
    void stopTone();
};

#endif // AUDIO_ENGINE_H