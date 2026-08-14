#pragma once
#include "raylib.h"

class AudioManager {
public:
    static AudioManager& Instance();

    void Init();
    void Close();

    void PlayDash();
    void PlayCollect();
    void PlayJump();
    void PlayHurt();
    void PlayLevelUp();
    void PlayClick();

private:
    AudioManager();
    ~AudioManager();

    bool initialized;

    Sound soundDash;
    Sound soundCollect;
    Sound soundJump;
    Sound soundHurt;
    Sound soundLevelUp;
    Sound soundClick;

    Sound CreateDashSound();
    Sound CreateCollectSound();
    Sound CreateJumpSound();
    Sound CreateHurtSound();
    Sound CreateLevelUpSound();
    Sound CreateClickSound();
};
