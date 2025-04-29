#include "AudioEngine.h"
#include <Arduino.h>

AudioEngine::AudioEngine() :
    buzzerPin(-1), initialized(false), thrustSoundActive(false),
    currentContinuousFreq(0), soundEndTime(0)
{}

void AudioEngine::begin(uint8_t pin) {
    buzzerPin = pin;
    stopTone(); // Ensure silence initially
    initialized = true;
    Serial.print("Audio Engine initialized on pin: ");
    Serial.println(pin);
    Serial.println("Using Arduino tone()/noTone()");
}

void AudioEngine::playTone(uint16_t freq, uint32_t duration) {
    if (!initialized) return;
    if (freq > 0) {
        if (duration > 0) {
            tone(buzzerPin, freq, duration);
            soundEndTime = millis() + duration + 5; // Add small buffer
            currentContinuousFreq = 0;
        } else {
            tone(buzzerPin, freq);
            soundEndTime = 0;
            currentContinuousFreq = freq;
        }
    } else {
        stopTone();
    }
}

void AudioEngine::stopTone() {
    if (!initialized) return;
    noTone(buzzerPin);
    currentContinuousFreq = 0;
    soundEndTime = 0;
}

void AudioEngine::playShootSound() {
    if (!initialized) return;
    // Stop continuous thrust if it's playing
    if (currentContinuousFreq >= SND_THRUST_FREQ_LOW && currentContinuousFreq <= SND_THRUST_FREQ_HIGH) {
         stopTone();
    }
    playTone(SND_SHOOT_FREQ, SND_SHORT_DURATION);
}

void AudioEngine::playExplosionSound() {
    if (!initialized) return;
    thrustSoundActive = false; // Stop thrust state
    // Stop continuous thrust if it's playing
    if (currentContinuousFreq >= SND_THRUST_FREQ_LOW && currentContinuousFreq <= SND_THRUST_FREQ_HIGH) {
        stopTone();
    }
    playTone(SND_EXPLODE_FREQ, SND_SHORT_DURATION * 2);
}

void AudioEngine::playHyperspaceSound() {
    if (!initialized) return;
    thrustSoundActive = false; // Stop thrust state
    // Stop continuous thrust if it's playing
    if (currentContinuousFreq >= SND_THRUST_FREQ_LOW && currentContinuousFreq <= SND_THRUST_FREQ_HIGH) {
        stopTone();
    }
    playTone(SND_HYPERSPACE_FREQ, SND_HYPERSPACE_DURATION);
}

void AudioEngine::startThrustSound(float intensity) {
    if (!initialized || thrustSoundActive) return;
    thrustSoundActive = true;

    intensity = max(0.0f, min(1.0f, intensity));
    uint16_t thrustFreq = SND_THRUST_FREQ_LOW + (uint16_t)((SND_THRUST_FREQ_HIGH - SND_THRUST_FREQ_LOW) * intensity);

    unsigned long currentTime = millis();
    if (soundEndTime == 0 || currentTime >= soundEndTime ) {
        playTone(thrustFreq, 0);
    }
}

void AudioEngine::stopThrustSound() {
    if (!initialized || !thrustSoundActive) return;
    thrustSoundActive = false;
    // Only stop if the current continuous tone is within the thrust range
    if (currentContinuousFreq >= SND_THRUST_FREQ_LOW && currentContinuousFreq <= SND_THRUST_FREQ_HIGH) {
        stopTone();
    }
}

void AudioEngine::stopAllSounds() {
     thrustSoundActive = false;
     stopTone();
}


void AudioEngine::update() {
    if (!initialized) return;
    unsigned long currentTime = millis();

    // Manage duration for short sounds
    if (soundEndTime > 0 && currentTime >= soundEndTime) {
        uint16_t freqJustEnded = currentContinuousFreq; // Might be 0 if tone() stopped it already
        currentContinuousFreq = 0; // Mark no continuous tone playing
        soundEndTime = 0; // Clear the end time

        // If thrust is supposed to be active now, restart it
        if (thrustSoundActive) {
             startThrustSound(1.0f); // Restart at default intensity
        }
    }
    // Backup check if thrust should be on but nothing playing
    else if (thrustSoundActive && currentContinuousFreq == 0 && soundEndTime == 0) {
        startThrustSound(1.0f); // Restart at default intensity
    }
}