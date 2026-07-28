#pragma once
#include "raylib.h"
#include "Player.h"
#include <vector>
#include <string>

enum BlockType {
    BLOCK_LONG,
    BLOCK_TALL,
    BLOCK_HAZARD
};


struct Block {
    Vector3 position;
    BlockType type;
    BoundingBox bounds;
};

class Map {
public:
    Map();
    ~Map();

    bool LoadLevelFromFile(const std::string& filepath);
    void ClearMap();
    void Draw();
    void HandleCollisions(Player& player);

    const std::vector<Block>& GetBlocks() const { return blocks; }

private:
    Model modelLong;
    Model modelTall;

    Texture2D texturePalette;
    std::vector<Block> blocks;

    void AddBlock(Vector3 pos, BlockType type);
};