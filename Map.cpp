#include "Map.h"
#include "raymath.h"

Map::Map() {
    modelLong = LoadModel("../resources/map/block-grass-overhang-long.obj");
    modelTall = LoadModel("../resources/map/block-grass-large-tall.obj");

    texturePalette = LoadTexture("../resources/map/colormap.png");

    if (modelLong.meshCount > 0) modelLong.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texturePalette;
    if (modelTall.meshCount > 0) modelTall.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texturePalette;

    // --- Ground & Spawn Area (Y = 0.0f) ---
    AddBlock((Vector3){ 10.0f, 0.0f, 0.0f }, BLOCK_LONG);
    AddBlock((Vector3){ 5.0f, 0.0f, 0.0f }, BLOCK_LONG);
    AddBlock((Vector3){ 15.0f, 0.0f, 0.0f }, BLOCK_LONG);
    AddBlock((Vector3){ 10.0f, 0.0f, 5.0f }, BLOCK_LONG);
    AddBlock((Vector3){ 10.0f, 0.0f, -5.0f }, BLOCK_LONG);

    // --- Tier 1 Parkour Steps (Y = 2.0f) ---
    // North Route
    AddBlock((Vector3){ 10.0f, 0.0f, -12.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 10.0f, 2.0f, -12.0f }, BLOCK_LONG);

    AddBlock((Vector3){ 10.0f, 0.0f, -20.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 10.0f, 2.0f, -20.0f }, BLOCK_LONG);

    AddBlock((Vector3){ 2.0f, 0.0f, -20.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 2.0f, 2.0f, -20.0f }, BLOCK_LONG);

    AddBlock((Vector3){ -6.0f, 0.0f, -20.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -6.0f, 2.0f, -20.0f }, BLOCK_LONG);

    // East Route
    AddBlock((Vector3){ 22.0f, 0.0f, 0.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 22.0f, 2.0f, 0.0f }, BLOCK_LONG);

    AddBlock((Vector3){ 30.0f, 0.0f, 0.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 30.0f, 2.0f, 0.0f }, BLOCK_LONG);

    AddBlock((Vector3){ 30.0f, 0.0f, 10.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 30.0f, 2.0f, 10.0f }, BLOCK_LONG);

    // --- Tier 2 Mid Parkour Steps (Y = 4.0f) ---
    // North-West Section
    AddBlock((Vector3){ -14.0f, 0.0f, -20.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -14.0f, 2.0f, -20.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -14.0f, 4.0f, -20.0f }, BLOCK_LONG);

    AddBlock((Vector3){ -22.0f, 0.0f, -20.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 2.0f, -20.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 4.0f, -20.0f }, BLOCK_LONG);

    AddBlock((Vector3){ -22.0f, 0.0f, -12.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 2.0f, -12.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 4.0f, -12.0f }, BLOCK_LONG);

    // East-South Section
    AddBlock((Vector3){ 30.0f, 0.0f, 20.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 30.0f, 2.0f, 20.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 30.0f, 4.0f, 20.0f }, BLOCK_LONG);

    AddBlock((Vector3){ 22.0f, 0.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 22.0f, 2.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 22.0f, 4.0f, 28.0f }, BLOCK_LONG);

    // --- Tier 3 High Towers & Sky Bridges (Y = 6.0f & Y = 8.0f) ---
    // High Tower West (Y = 6.0f -> 8.0f)
    AddBlock((Vector3){ -22.0f, 0.0f, -4.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 2.0f, -4.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 4.0f, -4.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 6.0f, -4.0f }, BLOCK_LONG);

    AddBlock((Vector3){ -22.0f, 0.0f, 4.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 2.0f, 4.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 4.0f, 4.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 6.0f, 4.0f }, BLOCK_TALL);
    AddBlock((Vector3){ -22.0f, 8.0f, 4.0f }, BLOCK_LONG);

    // High Tower South-East Summit (Y = 8.0f -> 10.0f)
    AddBlock((Vector3){ 14.0f, 0.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 14.0f, 2.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 14.0f, 4.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 14.0f, 6.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 14.0f, 8.0f, 28.0f }, BLOCK_LONG);

    AddBlock((Vector3){ 6.0f, 0.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 6.0f, 2.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 6.0f, 4.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 6.0f, 6.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 6.0f, 8.0f, 28.0f }, BLOCK_TALL);
    AddBlock((Vector3){ 6.0f, 10.0f, 28.0f }, BLOCK_LONG);
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