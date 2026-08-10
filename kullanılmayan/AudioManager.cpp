#include "AudioManager.h"
#include "raymath.h"
#include <cmath>
#include <cstdlib>

AudioManager& AudioManager::Instance() {
    static AudioManager instance;
    return instance;
}

AudioManager::AudioManager() : initialized(false) {
    soundDash = { 0 };
    soundCollect = { 0 };
    soundJump = { 0 };
    soundHurt = { 0 };
    soundLevelUp = { 0 };
    soundClick = { 0 };
}

AudioManager::~AudioManager() {
    Close();
}

void AudioManager::Init() {
    if (initialized) return;

    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;

    soundDash = CreateDashSound();
    soundCollect = CreateCollectSound();
    soundJump = CreateJumpSound();
    soundHurt = CreateHurtSound();
    soundLevelUp = CreateLevelUpSound();
    soundClick = CreateClickSound();

    initialized = true;
}

void AudioManager::Close() {
    if (!initialized) return;

    UnloadSound(soundDash);
    UnloadSound(soundCollect);
    UnloadSound(soundJump);
    UnloadSound(soundHurt);
    UnloadSound(soundLevelUp);
    UnloadSound(soundClick);

    CloseAudioDevice();
    initialized = false;
}

Sound AudioManager::CreateDashSound() {
    int sampleRate = 44100;
    int durationSamples = (int)(sampleRate * 0.20f); // 0.20 sec
    short *data = (short*)malloc(durationSamples * sizeof(short));

    for (int i = 0; i < durationSamples; i++) {
        float t = (float)i / sampleRate;
        float progress = (float)i / durationSamples;
        float freq = 400.0f * (1.0f - progress * 0.7f);
        float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float sine = sinf(2.0f * PI * freq * t);
        float envelope = sinf(PI * progress);
        float value = (sine * 0.5f + noise * 0.5f) * envelope * 0.8f;
        data[i] = (short)(value * 32000.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = durationSamples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave); // Frees data
    return s;
}

Sound AudioManager::CreateCollectSound() {
    int sampleRate = 44100;
    int durationSamples = (int)(sampleRate * 0.18f); // 0.18 sec
    short *data = (short*)malloc(durationSamples * sizeof(short));

    for (int i = 0; i < durationSamples; i++) {
        float t = (float)i / sampleRate;
        float progress = (float)i / durationSamples;
        float freq = (progress < 0.5f) ? 880.0f : 1320.0f; // A5 to E6 chime
        float sine = sinf(2.0f * PI * freq * t);
        float envelope = (1.0f - progress);
        float value = sine * envelope * 0.7f;
        data[i] = (short)(value * 32000.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = durationSamples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

Sound AudioManager::CreateJumpSound() {
    int sampleRate = 44100;
    int durationSamples = (int)(sampleRate * 0.15f);
    short *data = (short*)malloc(durationSamples * sizeof(short));

    for (int i = 0; i < durationSamples; i++) {
        float t = (float)i / sampleRate;
        float progress = (float)i / durationSamples;
        float freq = 250.0f + 400.0f * (progress * progress); // Pitch sweep up
        float sine = sinf(2.0f * PI * freq * t);
        float envelope = 1.0f - progress;
        float value = sine * envelope * 0.6f;
        data[i] = (short)(value * 32000.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = durationSamples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

Sound AudioManager::CreateHurtSound() {
    int sampleRate = 44100;
    int durationSamples = (int)(sampleRate * 0.22f);
    short *data = (short*)malloc(durationSamples * sizeof(short));

    for (int i = 0; i < durationSamples; i++) {
        float t = (float)i / sampleRate;
        float progress = (float)i / durationSamples;
        float freq = 120.0f * (1.0f - progress * 0.5f);
        float noise = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        float sqr = (sinf(2.0f * PI * freq * t) > 0.0f) ? 1.0f : -1.0f;
        float envelope = (1.0f - progress);
        float value = (sqr * 0.4f + noise * 0.6f) * envelope * 0.8f;
        data[i] = (short)(value * 32000.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = durationSamples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

Sound AudioManager::CreateLevelUpSound() {
    int sampleRate = 44100;
    int durationSamples = (int)(sampleRate * 0.45f);
    short *data = (short*)malloc(durationSamples * sizeof(short));

    // Arpeggio: C5 (523Hz), E5 (659Hz), G5 (784Hz), C6 (1046Hz)
    float freqs[] = { 523.25f, 659.25f, 784.00f, 1046.50f };

    for (int i = 0; i < durationSamples; i++) {
        float t = (float)i / sampleRate;
        float progress = (float)i / durationSamples;
        int noteIdx = (int)(progress * 4.0f);
        if (noteIdx > 3) noteIdx = 3;
        float freq = freqs[noteIdx];
        float sine = sinf(2.0f * PI * freq * t);
        float envelope = 1.0f - progress * 0.5f;
        float value = sine * envelope * 0.7f;
        data[i] = (short)(value * 32000.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = durationSamples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

Sound AudioManager::CreateClickSound() {
    int sampleRate = 44100;
    int durationSamples = (int)(sampleRate * 0.05f);
    short *data = (short*)malloc(durationSamples * sizeof(short));

    for (int i = 0; i < durationSamples; i++) {
        float t = (float)i / sampleRate;
        float progress = (float)i / durationSamples;
        float sine = sinf(2.0f * PI * 1200.0f * t);
        float envelope = 1.0f - progress;
        data[i] = (short)(sine * envelope * 0.5f * 32000.0f);
    }

    Wave wave = { 0 };
    wave.frameCount = durationSamples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;

    Sound s = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return s;
}

void AudioManager::PlayDash() {
    if (initialized && soundDash.stream.buffer != NULL) PlaySound(soundDash);
}

void AudioManager::PlayCollect() {
    if (initialized && soundCollect.stream.buffer != NULL) PlaySound(soundCollect);
}

void AudioManager::PlayJump() {
    if (initialized && soundJump.stream.buffer != NULL) PlaySound(soundJump);
}

void AudioManager::PlayHurt() {
    if (initialized && soundHurt.stream.buffer != NULL) PlaySound(soundHurt);
}

void AudioManager::PlayLevelUp() {
    if (initialized && soundLevelUp.stream.buffer != NULL) PlaySound(soundLevelUp);
}

void AudioManager::PlayClick() {
    if (initialized && soundClick.stream.buffer != NULL) PlaySound(soundClick);
}
