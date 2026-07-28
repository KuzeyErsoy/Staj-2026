#pragma once
#include "Player.h"
#include <string>

struct SaveData {
    int currentLevel;
    int exp;
    int maxExp;
    int level;
    int levelPoints;
    int hp;
    int maxHp;
    bool hasDoubleJump;
    float moveSpeedMultiplier;
    float dashCooldownMultiplier;
    float posX;
    float posY;
    float posZ;
};

class SaveSystem {
public:
    static bool SaveGame(const std::string& filepath, int currentLevel, const Player& player);
    static bool LoadGame(const std::string& filepath, int& currentLevel, Player& player);
    static bool SaveFileExists(const std::string& filepath);
};
