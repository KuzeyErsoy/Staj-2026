#include "SaveSystem.h"
#include <fstream>
#include "raylib.h"
#include <string>

bool SaveSystem::SaveGame(const std::string& filepath, int currentLevel, const Player& player) {
    SaveData data;
    data.currentLevel = currentLevel;
    data.exp = player.exp;
    data.maxExp = player.maxExp;
    data.level = player.level;
    data.levelPoints = player.levelPoints;
    data.hp = player.hp;
    data.maxHp = player.maxHp;
    data.hasDoubleJump = player.hasDoubleJump;
    data.moveSpeedMultiplier = player.moveSpeedMultiplier;
    data.dashCooldownMultiplier = player.dashCooldownMultiplier;
    data.posX = player.position.x;
    data.posY = player.position.y;
    data.posZ = player.position.z;

    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        TraceLog(LOG_WARNING, "SAVE: Failed to open save file '%s' for writing", filepath.c_str());
        return false;
    }

    file.write(reinterpret_cast<const char*>(&data), sizeof(SaveData));
    file.close();
    TraceLog(LOG_INFO, "SAVE: Successfully saved game state to '%s'", filepath.c_str());
    return true;
}

bool SaveSystem::LoadGame(const std::string& filepath, int& currentLevel, Player& player) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        TraceLog(LOG_WARNING, "SAVE: Failed to open save file '%s' for reading", filepath.c_str());
        return false;
    }

    SaveData data;
    file.read(reinterpret_cast<char*>(&data), sizeof(SaveData));
    file.close();

    currentLevel = data.currentLevel;
    player.exp = data.exp;
    player.maxExp = data.maxExp;
    player.level = data.level;
    player.levelPoints = data.levelPoints;
    player.hp = data.hp;
    player.maxHp = data.maxHp;
    player.hasDoubleJump = data.hasDoubleJump;
    if (player.hasDoubleJump) player.maxJumps = 2;
    player.moveSpeedMultiplier = data.moveSpeedMultiplier;
    player.dashCooldownMultiplier = data.dashCooldownMultiplier;
    player.position = (Vector3){ data.posX, data.posY, data.posZ };

    TraceLog(LOG_INFO, "SAVE: Successfully loaded game state from '%s'", filepath.c_str());
    return true;
}

bool SaveSystem::SaveFileExists(const std::string& filepath) {
    return FileExists(filepath.c_str());
}
