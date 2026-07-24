#pragma once
#include "raylib.h"
#include "Player.h"
#include <vector>

enum BlockType {
    BLOCK_LONG,
    BLOCK_TALL
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

    void Draw();
    void HandleCollisions(Player& player);

private:
    Model modelLong;
    Model modelTall;

    Texture2D texturePalette;
    std::vector<Block> blocks;

    void AddBlock(Vector3 pos, BlockType type);
};