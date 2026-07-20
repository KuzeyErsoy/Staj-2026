#include "Map.h"
#include "raymath.h"

Map::Map() {
    modelLong = LoadModel("../resources/map/block-grass-overhang-long.obj");
    modelSlope = LoadModel("../resources/map/block-grass-large-slope.obj");
    modelTall = LoadModel("../resources/map/block-grass-large-tall.obj");

    texturePalette = LoadTexture("../resources/map/colormap.png");

    if (modelLong.meshCount > 0) modelLong.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texturePalette;
    if (modelSlope.meshCount > 0) modelSlope.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texturePalette;
    if (modelTall.meshCount > 0) modelTall.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texturePalette;
    

    AddBlock((Vector3){ 0.0f, 0.0f, 0.0f }, BLOCK_TALL); // Starting block for player spawn
// Base Ground Area (Starting area, 2x2 blocks)
    AddBlock((Vector3){ 0.0f, 0.0f, 0.0f }, BLOCK_LONG);
    AddBlock((Vector3){ 0.0f, 0.0f, -5.0f }, BLOCK_LONG);
    AddBlock((Vector3){ 5.0f, 0.0f, 0.0f }, BLOCK_LONG);
    AddBlock((Vector3){ 5.0f, 0.0f, -5.0f }, BLOCK_LONG);

    // First Ramp (Going up)
    AddBlock((Vector3){ 0.0f, 0.0f, -10.0f }, BLOCK_SLOPE);
    AddBlock((Vector3){ 5.0f, 0.0f, -10.0f }, BLOCK_SLOPE);

    // First Elevation Level (Supported from underneath)
    AddBlock((Vector3){ 0.0f, 0.0f, -15.0f }, BLOCK_TALL); // Support block
    AddBlock((Vector3){ 0.0f, 2.0f, -15.0f }, BLOCK_LONG); // Walkable surface
    
    AddBlock((Vector3){ 5.0f, 0.0f, -15.0f }, BLOCK_TALL); // Support block
    AddBlock((Vector3){ 5.0f, 2.0f, -15.0f }, BLOCK_LONG); // Walkable surface

    // Second Ramp (Steeper/Higher test)
    AddBlock((Vector3){ 0.0f, 2.0f, -20.0f }, BLOCK_SLOPE);

    // Highest Elevation Level (Double supported)
    AddBlock((Vector3){ 0.0f, 0.0f, -25.0f }, BLOCK_TALL); // Base support
    AddBlock((Vector3){ 0.0f, 2.0f, -25.0f }, BLOCK_TALL); // Mid support
    AddBlock((Vector3){ 0.0f, 4.0f, -25.0f }, BLOCK_LONG); // Walkable top surface
}

Map::~Map() {
    UnloadTexture(texturePalette);

    UnloadModel(modelLong);
    UnloadModel(modelSlope);
    UnloadModel(modelTall);
}

void Map::AddBlock(Vector3 pos, BlockType type) {
    Block b;
    b.position = pos;
    b.type = type;

    BoundingBox baseBounds;
    if (type == BLOCK_LONG) baseBounds = GetModelBoundingBox(modelLong);
    else if (type == BLOCK_SLOPE) baseBounds = GetModelBoundingBox(modelSlope);
    else if (type == BLOCK_TALL) baseBounds = GetModelBoundingBox(modelTall);

    b.bounds.min = Vector3Add(baseBounds.min, pos);
    b.bounds.max = Vector3Add(baseBounds.max, pos);

    blocks.push_back(b);
}

void Map::Draw() {
    for (const auto& block : blocks) {
        if (block.type == BLOCK_LONG) {
            DrawModel(modelLong, block.position, 1.0f, WHITE);
        } else if (block.type == BLOCK_SLOPE) {
            DrawModel(modelSlope, block.position, 1.0f, WHITE);
        } else if (block.type == BLOCK_TALL) {
            DrawModel(modelTall, block.position, 1.0f, WHITE);
        }
    }
}

void Map::HandleCollisions(Player& player) {
    BoundingBox pBounds = player.GetBounds();
    bool hitFloor = false;

    for (const auto& block : blocks) {
        if (CheckCollisionBoxes(pBounds, block.bounds)) {
            if (player.velocity.y < 0.0f && (player.position.y - player.velocity.y) >= block.bounds.max.y) {
                player.position.y = block.bounds.max.y;
                player.velocity.y = 0.0f;
                hitFloor = true;
            } else {
                player.position.x = player.prevPosition.x;
                player.position.z = player.prevPosition.z;
                pBounds = player.GetBounds();
            }
        }
    }

    if (hitFloor) {
        player.isGrounded = true;
    }
}