#include "Map.h"
#include "raymath.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

Map::Map() {
    if (FileExists("resources/map/block-grass-overhang-long.obj")) {
        modelLong = LoadModel("resources/map/block-grass-overhang-long.obj");
        modelTall = LoadModel("resources/map/block-grass-large-tall.obj");
        texturePalette = LoadTexture("resources/map/colormap.png");
    } else {
        modelLong = LoadModel("../resources/map/block-grass-overhang-long.obj");
        modelTall = LoadModel("../resources/map/block-grass-large-tall.obj");
        texturePalette = LoadTexture("../resources/map/colormap.png");
    }

    if (modelLong.meshCount > 0) modelLong.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texturePalette;
    if (modelTall.meshCount > 0) modelTall.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texturePalette;

    if (!LoadLevelFromFile("resources/map/level1.txt")) {
        LoadLevelFromFile("../resources/map/level1.txt");
    }
}

void Map::ClearMap() {
    blocks.clear();
}

bool Map::LoadLevelFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        TraceLog(LOG_WARNING, "MAP: Could not open level file '%s'", filepath.c_str());
        return false;
    }

    ClearMap();

    std::string line;
    int row = 0;
    const float blockSize = 5.0f;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string token;
        int col = 0;

        while (ss >> token) {
            if (token == ".") {
                col++;
                continue;
            }

            float x = (col - 4) * blockSize;
            float z = (row - 4) * blockSize;

            char typeChar = token[0];
            int height = 0;
            if (token.length() > 1 && std::isdigit(token[1])) {
                height = token[1] - '0';
            }

            if (typeChar == 'L') {
                for (int h = 0; h < height; ++h) {
                    AddBlock((Vector3){ x, h * 2.0f, z }, BLOCK_TALL);
                }
                AddBlock((Vector3){ x, height * 2.0f, z }, BLOCK_LONG);
            } else if (typeChar == 'T') {
                int totalTall = (height == 0) ? 1 : height;
                for (int h = 0; h < totalTall; ++h) {
                    AddBlock((Vector3){ x, h * 2.0f, z }, BLOCK_TALL);
                }
            }

            col++;
        }
        row++;
    }

    TraceLog(LOG_INFO, "MAP: Successfully loaded %d blocks from level file '%s'", (int)blocks.size(), filepath.c_str());
    return true;
}

Map::~Map() {
    UnloadTexture(texturePalette);

    UnloadModel(modelLong);
    UnloadModel(modelTall);
}

void Map::AddBlock(Vector3 pos, BlockType type) {
    Block b;
    b.position = pos;
    b.type = type;

    BoundingBox baseBounds;
    if (type == BLOCK_LONG) baseBounds = GetModelBoundingBox(modelLong);
    else if (type == BLOCK_TALL) baseBounds = GetModelBoundingBox(modelTall);

    b.bounds.min = Vector3Add(baseBounds.min, pos);
    b.bounds.max = Vector3Add(baseBounds.max, pos);

    blocks.push_back(b);
}

void Map::Draw() {
    for (const auto& block : blocks) {
        if (block.type == BLOCK_LONG) {
            DrawModel(modelLong, block.position, 1.0f, WHITE);
        } else if (block.type == BLOCK_TALL) {
            DrawModel(modelTall, block.position, 1.0f, WHITE);
        }
    }
}

void Map::HandleCollisions(Player& player) {
    BoundingBox pBounds = player.GetBounds();
    bool hitFloor = false;

    // Reset grounded check for air time (unless staying on ground plane)
    player.isGrounded = (player.position.y <= 0.0f);

    const float stepAllowance = 0.5f;

    for (const auto& block : blocks) {
        pBounds = player.GetBounds();
        if (CheckCollisionBoxes(pBounds, block.bounds)) {
            bool isAboveBlockTop = (player.position.y >= block.bounds.max.y - stepAllowance);

            if (isAboveBlockTop) {
                // Over the top surface of the block
                if (player.velocity.y <= 0.1f) {
                    player.position.y = block.bounds.max.y;
                    player.velocity.y = 0.0f;
                    hitFloor = true;
                }
            } else {
                // Side wall collision pushback
                player.position.x = player.prevPosition.x;
                player.position.z = player.prevPosition.z;

                // Head bump check if rising under a block
                if (player.velocity.y > 0.0f && pBounds.max.y >= block.bounds.min.y) {
                    player.velocity.y = 0.0f;
                }
            }
        }
    }

    if (hitFloor) {
        player.isGrounded = true;
    }
}