#include "FleetHUD.h"
#include "Drone.h"
#include "FleetCamera.h"
#include "ClusterManager.h"
#include "AudioManager.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

namespace DroneFleet {

FleetHUD::FleetHUD() : fontLoaded(false), pipTextureLoaded(false) {
    Init();
}

FleetHUD::~FleetHUD() {
    if (fontLoaded) {
        UnloadFont(customFont);
    }
    if (pipTextureLoaded) {
        UnloadRenderTexture(pipTexture);
        pipTextureLoaded = false;
    }
}

void FleetHUD::Init() {
    const char* fontPaths[] = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "resources/fonts/arial.ttf",
        "../resources/fonts/arial.ttf"
    };

    for (const char* path : fontPaths) {
        if (FileExists(path)) {
            customFont = LoadFontEx(path, 36, NULL, 0);
            if (customFont.texture.id > 0) {
                GenTextureMipmaps(&customFont.texture);
                SetTextureFilter(customFont.texture, TEXTURE_FILTER_TRILINEAR);
                fontLoaded = true;
                break;
            }
        }
    }

    if (!pipTextureLoaded) {
        pipTexture = LoadRenderTexture(340, 200);
        if (pipTexture.id > 0) {
            SetTextureFilter(pipTexture.texture, TEXTURE_FILTER_BILINEAR);
            pipTextureLoaded = true;
        }
    }
}


void FleetHUD::DrawCustomText(const char* text, float posX, float posY, float fontSize, Color color) const {
    if (fontLoaded) {
        DrawTextEx(customFont, text, Vector2{ posX, posY }, fontSize, 1.0f, color);
    } else {
        DrawText(text, (int)posX, (int)posY, (int)fontSize, color);
    }
}

void FleetHUD::ToggleAllPanels() {
    bool anyOn = showTelemetryPanel || showResourcePanel || showTerminalPanel || showCameraBanner || showTacticalMap;
    showTelemetryPanel = !anyOn;
    showResourcePanel = !anyOn;
    showTerminalPanel = !anyOn;
    showCameraBanner = !anyOn;
    showTacticalMap = !anyOn;
}

void FleetHUD::ToggleThermalFLIR() {
    if (visionMode == VisionMode::FLIR_THERMAL) {
        visionMode = VisionMode::NORMAL;
    } else {
        visionMode = VisionMode::FLIR_THERMAL;
    }
}

void FleetHUD::ToggleNightVision() {
    if (visionMode == VisionMode::NVG_GREEN) {
        visionMode = VisionMode::NORMAL;
    } else {
        visionMode = VisionMode::NVG_GREEN;
    }
}

void FleetHUD::DrawSNRCoverageRings2D(int centerX, int centerY, float scale, bool fullscreen) const {
    // 200m Ring: Green (Optimal Signal SNR > 25 dB)
    DrawCircleLines(centerX, centerY, (int)(200 * scale), Fade(LIME, 0.40f));
    DrawCircleLines(centerX, centerY, (int)(200 * scale) - 1, Fade(LIME, 0.20f));

    // 400m Ring: Yellow (Standard Signal SNR 18-25 dB)
    DrawCircleLines(centerX, centerY, (int)(400 * scale), Fade(YELLOW, 0.35f));
    DrawCircleLines(centerX, centerY, (int)(400 * scale) - 1, Fade(YELLOW, 0.18f));

    // 600m Ring: Orange (Degraded Signal SNR 12-18 dB)
    DrawCircleLines(centerX, centerY, (int)(600 * scale), Fade(ORANGE, 0.30f));

    // 800m Ring: Red (Fringe / Packet Loss SNR < 10 dB)
    DrawCircleLines(centerX, centerY, (int)(800 * scale), Fade(RED, 0.25f));

    if (fullscreen) {
        DrawCustomText("200m [SNR 45-25dB OPTIMAL LINK]", (float)(centerX + 12), (float)(centerY - (int)(200 * scale) - 12), 10.0f, Fade(LIME, 0.85f));
        DrawCustomText("400m [SNR 25-18dB STANDARD LINK]", (float)(centerX + 12), (float)(centerY - (int)(400 * scale) - 12), 10.0f, Fade(YELLOW, 0.85f));
        DrawCustomText("600m [SNR 18-12dB WEAK / LATENCY]", (float)(centerX + 12), (float)(centerY - (int)(600 * scale) - 12), 10.0f, Fade(ORANGE, 0.85f));
        DrawCustomText("800m [SNR <10dB PACKET LOSS ZONE]", (float)(centerX + 12), (float)(centerY - (int)(800 * scale) - 12), 10.0f, Fade(RED, 0.85f));
    }
}

void FleetHUD::DrawMapFilterBar2D(int barX, int barY, int barW, int barH, const std::vector<std::unique_ptr<Drone>>& drones, const std::vector<EnemyCluster>& clusters) const {
    int reconCount = 0, tfCount = 0, supCount = 0, totalDrones = 0;
    for (const auto& d : drones) {
        if (!d->isDestroyed) {
            totalDrones++;
            if (d->role == DroneRole::RECON) reconCount++;
            else if (d->role == DroneRole::TASK_FORCE) tfCount++;
            else if (d->role == DroneRole::SUPPORT) supCount++;
        }
    }
    int hostileClusters = 0;
    for (const auto& c : clusters) {
        if (c.status != ClusterStatus::CLEARED) hostileClusters++;
    }

    struct FilterBtn {
        MapFilterMode mode;
        const char* label;
        Color col;
        int hotkey;
    };

    FilterBtn btns[5] = {
        { MapFilterMode::ALL_DRONES, TextFormat("[1] ALL ASSETS (%d)", totalDrones), SKYBLUE, KEY_ONE },
        { MapFilterMode::RECON_ONLY, TextFormat("[2] RECON SCOUTS (%d)", reconCount), SKYBLUE, KEY_TWO },
        { MapFilterMode::TASK_FORCE_ONLY, TextFormat("[3] TASK FORCE (%d)", tfCount), ORANGE, KEY_THREE },
        { MapFilterMode::SUPPORT_ONLY, TextFormat("[4] SUPPORT (%d)", supCount), LIME, KEY_FOUR },
        { MapFilterMode::HOSTILES_ONLY, TextFormat("[5] HOSTILES (%d)", hostileClusters), RED, KEY_FIVE }
    };

    Vector2 mousePos = GetMousePosition();
    bool mouseClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    int btnW = (barW - 40) / 5;
    int curX = barX;

    for (int i = 0; i < 5; i++) {
        Rectangle btnRect = { (float)curX, (float)barY, (float)btnW, (float)barH };
        bool hovered = CheckCollisionPointRec(mousePos, btnRect);
        bool selected = (activeMapFilter == btns[i].mode);

        if (hovered && mouseClicked) {
            activeMapFilter = btns[i].mode;
            if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
        }
        if (IsKeyPressed(btns[i].hotkey)) {
            activeMapFilter = btns[i].mode;
            if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
        }

        Color bgCol = selected ? Color{ 16, 36, 68, 250 } : (hovered ? Color{ 10, 24, 45, 230 } : Color{ 4, 12, 24, 210 });
        Color borderCol = selected ? btns[i].col : (hovered ? RAYWHITE : Fade(btns[i].col, 0.45f));

        DrawRectangleRec(btnRect, bgCol);
        DrawRectangleLinesEx(btnRect, selected ? 2.0f : 1.0f, borderCol);

        float textW = fontLoaded ? MeasureTextEx(customFont, btns[i].label, 11.0f, 1.0f).x : (float)MeasureText(btns[i].label, 11);
        DrawCustomText(btns[i].label, (float)(curX + (btnW - (int)textW) / 2), (float)(barY + (barH - 12) / 2), 11.0f, selected ? RAYWHITE : borderCol);

        curX += btnW + 10;
    }
}

void FleetHUD::DrawTargetAnalysisCard2D(int cardX, int cardY, int cardW, int cardH, const EnemyCluster& cluster, const std::vector<std::unique_ptr<Drone>>& drones) const {
    DrawRectangle(cardX, cardY, cardW, cardH, Color{ 4, 11, 24, 252 });
    Color frameCol = (cluster.armedCount > 0) ? Color{ 255, 60, 50, 255 } : Color{ 255, 160, 30, 255 };
    DrawRectangleLines(cardX, cardY, cardW, cardH, frameCol);
    DrawRectangleLines(cardX + 2, cardY + 2, cardW - 4, cardH - 4, Fade(frameCol, 0.4f));

    // Corner brackets
    int cLen = 14;
    DrawLine(cardX - 2, cardY - 2, cardX - 2 + cLen, cardY - 2, frameCol);
    DrawLine(cardX - 2, cardY - 2, cardX - 2, cardY - 2 + cLen, frameCol);
    DrawLine(cardX + cardW + 2, cardY - 2, cardX + cardW + 2 - cLen, cardY - 2, frameCol);
    DrawLine(cardX + cardW + 2, cardY - 2, cardX + cardW + 2, cardY - 2 + cLen, frameCol);

    DrawCustomText("TARGET CLUSTER ANALYSIS", (float)(cardX + 15), (float)(cardY + 12), 14.0f, frameCol);
    DrawLine(cardX + 15, cardY + 30, cardX + cardW - 15, cardY + 30, Fade(frameCol, 0.5f));

    int cy = cardY + 38;
    auto DrawRow = [&](const char* label, const char* val, Color col) {
        DrawCustomText(label, (float)(cardX + 15), (float)cy, 11.0f, LIGHTGRAY);
        float vW = fontLoaded ? MeasureTextEx(customFont, val, 11.0f, 1.0f).x : (float)MeasureText(val, 11);
        DrawCustomText(val, (float)(cardX + cardW - 15 - (int)vW), (float)cy, 11.0f, col);
        cy += 19;
    };

    DrawRow("Target Identifier:", TextFormat("HOSTILE CLUSTER #%d", cluster.id), frameCol);
    
    const char* statStr = (cluster.status == ClusterStatus::UNDETECTED) ? "UNDETECTED // ESTIMATED" :
                          ((cluster.status == ClusterStatus::REPORTED) ? "CONFIRMED // SCOUTED" : "ENGAGED // COMBAT ACTIVE");
    Color statCol = (cluster.status == ClusterStatus::UNDETECTED) ? YELLOW : ((cluster.status == ClusterStatus::REPORTED) ? ORANGE : RED);
    DrawRow("Intelligence Status:", statStr, statCol);

    DrawRow("Grid Coordinates:", TextFormat("X: %+.0fm | Z: %+.0fm", cluster.centerPosition.x, cluster.centerPosition.z), SKYBLUE);
    float distToBase = Vector3Length(Vector3{ cluster.centerPosition.x, 0.0f, cluster.centerPosition.z });
    DrawRow("Distance to Base:", TextFormat("%.0f meters", distToBase), RAYWHITE);

    // Threat Breakdown
    cy += 4;
    DrawLine(cardX + 15, cy, cardX + cardW - 15, cy, Fade(GRAY, 0.3f));
    cy += 8;
    DrawCustomText("THREAT & FORCE ESTIMATION:", (float)(cardX + 15), (float)cy, 11.0f, GOLD);
    cy += 18;

    DrawRow("Calculated Threat Mag:", TextFormat("%d pts", cluster.calculatedThreatMagnitude), (cluster.calculatedThreatMagnitude >= 15) ? RED : YELLOW);
    DrawRow("Total Combatants:", TextFormat("%d Units", cluster.remainingMagnitude), RAYWHITE);
    DrawRow("Armed Attack Drones:", TextFormat("%d Units (0.5x Threat)", cluster.armedCount), RED);
    DrawRow("Unarmed Logistics Drones:", TextFormat("%d Units (0.25x Threat)", cluster.unarmedCount), ORANGE);

    // Strike Doctrine Recommendation
    cy += 4;
    DrawLine(cardX + 15, cy, cardX + cardW - 15, cy, Fade(GRAY, 0.3f));
    cy += 8;
    DrawCustomText("RECOMMENDED STRIKE DOCTRINE:", (float)(cardX + 15), (float)cy, 11.0f, LIME);
    cy += 18;

    const char* doctrineStr = (cluster.armedCount > 6) ? "3x Task Force Pincer + 1x Support Medic" :
                              ((cluster.armedCount > 0) ? "2x Task Force Pincer Assault Salvo" : "1x Recon Harass / Rapid Intercept");
    DrawCustomText(doctrineStr, (float)(cardX + 15), (float)cy, 10.0f, RAYWHITE);
    cy += 22;

    // Assigned Assets
    DrawCustomText("DISPATCHED FLEET ASSETS:", (float)(cardX + 15), (float)cy, 11.0f, SKYBLUE);
    cy += 18;
    int assignedCount = 0;
    for (const auto& d : drones) {
        if (d->assignedClusterId == cluster.id && !d->isDestroyed) {
            assignedCount++;
            float dist = Vector3Distance(d->position, cluster.centerPosition);
            const char* rStr = (d->role == DroneRole::TASK_FORCE) ? "TF" : ((d->role == DroneRole::SUPPORT) ? "SUP" : "REC");
            DrawCustomText(TextFormat("• Drone #%d [%s] - Range: %.0fm (ETA ~%.0fs)", d->id, rStr, dist, dist / 25.0f), (float)(cardX + 18), (float)cy, 10.0f, LIME);
            cy += 18;
        }
    }
    if (assignedCount == 0) {
        DrawCustomText("• No drones assigned (Standing by)", (float)(cardX + 18), (float)cy, 10.0f, GRAY);
        cy += 18;
    }

    // Close button prompt
    DrawLine(cardX + 15, cardY + cardH - 30, cardX + cardW - 15, cardY + cardH - 30, Fade(frameCol, 0.4f));
    DrawCustomText("[X] / [Right-Click] Deselect Target Card", (float)(cardX + 15), (float)(cardY + cardH - 22), 10.0f, GOLD);
}

void FleetHUD::DrawMapDroneQuickCard2D(int cardX, int cardY, int cardW, int cardH, const Drone& drone) const {
    DrawRectangle(cardX, cardY, cardW, cardH, Color{ 4, 11, 24, 252 });
    Color roleCol = (drone.role == DroneRole::RECON) ? SKYBLUE : ((drone.role == DroneRole::TASK_FORCE) ? ORANGE : LIME);
    DrawRectangleLines(cardX, cardY, cardW, cardH, roleCol);
    DrawRectangleLines(cardX + 2, cardY + 2, cardW - 4, cardH - 4, Fade(roleCol, 0.4f));

    const char* rName = (drone.role == DroneRole::RECON) ? "RECON SCOUT" : ((drone.role == DroneRole::TASK_FORCE) ? "TASK FORCE STRIKE" : "SUPPORT TANKER");
    DrawCustomText(TextFormat("DRONE #%d - %s", drone.id, rName), (float)(cardX + 15), (float)(cardY + 12), 14.0f, roleCol);
    DrawLine(cardX + 15, cardY + 30, cardX + cardW - 15, cardY + 30, Fade(roleCol, 0.5f));

    int cy = cardY + 38;
    auto DrawRow = [&](const char* label, const char* val, Color col) {
        DrawCustomText(label, (float)(cardX + 15), (float)cy, 11.0f, LIGHTGRAY);
        float vW = fontLoaded ? MeasureTextEx(customFont, val, 11.0f, 1.0f).x : (float)MeasureText(val, 11);
        DrawCustomText(val, (float)(cardX + cardW - 15 - (int)vW), (float)cy, 11.0f, col);
        cy += 19;
    };

    DrawRow("Telemetry Status:", drone.isDestroyed ? "CRASHED / SAR" : ((drone.state == DroneState::IDLE_AT_BASE) ? "DOCKED AT BASE" : "IN FLIGHT"), drone.isDestroyed ? RED : LIME);
    DrawRow("Grid Coordinates:", TextFormat("X: %+.0fm | Z: %+.0fm", drone.position.x, drone.position.z), RAYWHITE);
    DrawRow("Current Altitude:", TextFormat("%.1f meters (y)", drone.position.y), (drone.position.y < 8.0f) ? YELLOW : SKYBLUE);
    DrawRow("Air Speed:", TextFormat("%.1f m/s", Vector3Length(drone.velocity)), LIGHTGRAY);

    DrawRow("SNR Link Quality:", TextFormat("%.1f dB (%.0f%% Link)", drone.snrDb, drone.signalQuality * 100.0f), (drone.snrDb > 25.0f) ? LIME : ((drone.snrDb > 15.0f) ? YELLOW : RED));
    DrawRow("Hull Integrity:", TextFormat("%.0f / %.0f HP", drone.hp, drone.maxHp), (drone.hp > 100.0f) ? LIME : RED);
    DrawRow("Fuel Level:", TextFormat("%.0f / %.0f Liters", drone.fuel, drone.maxFuel), YELLOW);
    if (drone.role != DroneRole::RECON) {
        DrawRow("Ammunition:", TextFormat("%.0f / %.0f Rounds", drone.ammo, drone.maxAmmo), RED);
    }

    if (drone.assignedClusterId != -1) {
        DrawRow("Assigned Mission:", TextFormat("Engaging Cluster #%d", drone.assignedClusterId), GOLD);
    } else {
        DrawRow("Assigned Mission:", "Patrol / Standby", LIGHTGRAY);
    }

    DrawLine(cardX + 15, cardY + cardH - 30, cardX + cardW - 15, cardY + cardH - 30, Fade(roleCol, 0.4f));
    DrawCustomText("[TAB] Track Cam | [X] Deselect", (float)(cardX + 15), (float)(cardY + cardH - 22), 10.0f, GOLD);
}

void FleetHUD::DrawTacticalMap2D(int mapX, int mapY, int mapW, int mapH, bool fullscreen, const std::vector<std::unique_ptr<Drone>>& drones, const std::vector<EnemyCluster>& clusters) const {
    DrawRectangle(mapX, mapY, mapW, mapH, Color{ 3, 8, 16, 250 });
    DrawRectangleLines(mapX, mapY, mapW, mapH, SKYBLUE);
    DrawRectangleLines(mapX + 2, mapY + 2, mapW - 4, mapH - 4, Fade(SKYBLUE, 0.35f));

    Vector2 mousePos = GetMousePosition();
    bool mouseClicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool rightClicked = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);

    if (IsKeyPressed(KEY_X) || rightClicked) {
        selectedClusterId = -1;
        selectedMapDroneId = -1;
    }

    // Header & Filter Bar
    if (fullscreen) {
        DrawCustomText("TACTICAL RADAR ARENA MAP - 1000x1000m THEATER GRID [SNR TELEMETRY ACTIVE]",
                       (float)(mapX + 18), (float)(mapY + 14), 17.0f, SKYBLUE);

        // Filter Bar (Top Right Area)
        int filterBarW = 680;
        int filterBarH = 28;
        int filterBarX = mapX + mapW - filterBarW - 18;
        int filterBarY = mapY + 12;
        DrawMapFilterBar2D(filterBarX, filterBarY, filterBarW, filterBarH, drones, clusters);
    } else {
        DrawCustomText("TACTICAL MAP", (float)(mapX + 15), (float)(mapY + 10), 14.0f, SKYBLUE);
    }

    float scale = (float)mapW / 1000.0f;
    int centerX = mapX + mapW / 2;
    int centerY = mapY + mapH / 2;

    if (fullscreen) {
        // Offset center slightly right if left-side analysis card is open
        if (selectedClusterId != -1 || selectedMapDroneId != -1) {
            centerX = mapX + 180 + (mapW - 180) / 2;
        }
    }

    // 1. Tactical Grid Lines & Distance Rings
    for (int g = -400; g <= 400; g += 200) {
        if (g == 0) continue;
        int gx = centerX + (int)(g * scale);
        int gy = centerY + (int)(g * scale);
        DrawLine(gx, mapY + 50, gx, mapY + mapH - 30, Fade(SKYBLUE, 0.08f));
        DrawLine(mapX + 30, gy, mapX + mapW - 30, gy, Fade(SKYBLUE, 0.08f));
        if (fullscreen) {
            DrawText(TextFormat("%+dm", g), gx + 3, centerY + 3, 9, Fade(SKYBLUE, 0.35f));
            DrawText(TextFormat("%+dm", -g), centerX + 3, gy - 11, 9, Fade(SKYBLUE, 0.35f));
        }
    }

    DrawLine(mapX + 20, centerY, mapX + mapW - 20, centerY, Fade(SKYBLUE, 0.25f));
    DrawLine(centerX, mapY + 45, centerX, mapY + mapH - 20, Fade(SKYBLUE, 0.25f));

    if (fullscreen) {
        // Cardinal direction annotations
        DrawCustomText("[N] 000°", (float)(centerX - 18), (float)(mapY + 48), 10.0f, SKYBLUE);
        DrawCustomText("[S] 180°", (float)(centerX - 18), (float)(mapY + mapH - 36), 10.0f, SKYBLUE);
        DrawCustomText("[E] 090°", (float)(mapX + mapW - 65), (float)(centerY - 6), 10.0f, SKYBLUE);
        DrawCustomText("[W] 270°", (float)(mapX + 25), (float)(centerY - 6), 10.0f, SKYBLUE);
    }

    // 2. SNR Coverage Concentric Rings (200m, 400m, 600m, 800m)
    DrawSNRCoverageRings2D(centerX, centerY, scale, fullscreen);

    // 3. Central Hangar Base Marker
    DrawRectangleLines(centerX - 7, centerY - 7, 14, 14, SKYBLUE);
    DrawRectangle(centerX - 4, centerY - 4, 8, 8, Fade(SKYBLUE, 0.4f));
    DrawCustomText("BASE", (float)(centerX - 12), (float)(centerY - 20), 10.0f, SKYBLUE);

    // 4. Draw Flight Trajectory Paths & Destination Vectors for Drones
    if (fullscreen) {
        for (const auto& d : drones) {
            if (d->isDestroyed || d->state == DroneState::IDLE_AT_BASE) continue;

            // Check filter visibility
            if (activeMapFilter == MapFilterMode::RECON_ONLY && d->role != DroneRole::RECON) continue;
            if (activeMapFilter == MapFilterMode::TASK_FORCE_ONLY && d->role != DroneRole::TASK_FORCE) continue;
            if (activeMapFilter == MapFilterMode::SUPPORT_ONLY && d->role != DroneRole::SUPPORT) continue;
            if (activeMapFilter == MapFilterMode::HOSTILES_ONLY) continue;

            int dx = centerX + (int)(d->position.x * scale);
            int dy = centerY + (int)(d->position.z * scale);
            int tx = centerX + (int)(d->targetPosition.x * scale);
            int ty = centerY + (int)(d->targetPosition.z * scale);

            Color trajCol = (d->role == DroneRole::RECON) ? Fade(SKYBLUE, 0.35f) :
                            ((d->role == DroneRole::TASK_FORCE) ? Fade(ORANGE, 0.40f) : Fade(LIME, 0.35f));

            DrawLine(dx, dy, tx, ty, trajCol);
            DrawCircleLines(tx, ty, 3, trajCol);
        }
    }

    // 5. Draw Hostile Enemy Clusters
    for (const auto& c : clusters) {
        if (c.status == ClusterStatus::CLEARED) continue;

        int cx = centerX + (int)(c.centerPosition.x * scale);
        int cy = centerY + (int)(c.centerPosition.z * scale);

        Color cCol = (c.armedCount > 0) ? Color{ 255, 60, 50, 255 } : Color{ 255, 170, 30, 255 };
        if (c.status == ClusterStatus::UNDETECTED) cCol = YELLOW;
        else if (c.status == ClusterStatus::REPORTED) cCol = ORANGE;

        // Hover & Click Detection
        float distToMouse = Vector2Distance(mousePos, Vector2{ (float)cx, (float)cy });
        bool isHovered = (distToMouse < 14.0f);

        if (fullscreen && isHovered && mouseClicked) {
            selectedClusterId = c.id;
            selectedMapDroneId = -1;
            if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
        }

        // Draw Cluster Icon & Pulse
        float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 4.0f + (float)c.id);
        DrawCircle(cx, cy, isHovered ? 7 : 5, cCol);
        DrawCircleLines(cx, cy, (int)(8 + pulse * 4), Fade(cCol, 0.6f));

        if (fullscreen) {
            DrawText(TextFormat("#%d [Mag:%d]", c.id, c.calculatedThreatMagnitude), cx + 9, cy - 6, 11, RAYWHITE);
            const char* typeTag = (c.armedCount > 0) ? "ARMED" : "UNARM";
            DrawText(typeTag, cx + 9, cy + 5, 9, (c.armedCount > 0) ? RED : ORANGE);
        }

        // Selected Target Bracket
        if (selectedClusterId == c.id) {
            int bSize = 14;
            DrawRectangleLines(cx - bSize, cy - bSize, bSize * 2, bSize * 2, GOLD);
            DrawCircleLines(cx, cy, bSize + 4, Fade(GOLD, 0.5f));
        }
    }

    // 6. Draw Drones on Map
    for (const auto& d : drones) {
        int dx = centerX + (int)(d->position.x * scale);
        int dy = centerY + (int)(d->position.z * scale);

        // Filter Check
        bool isDimmed = false;
        if (activeMapFilter == MapFilterMode::RECON_ONLY && d->role != DroneRole::RECON) isDimmed = true;
        else if (activeMapFilter == MapFilterMode::TASK_FORCE_ONLY && d->role != DroneRole::TASK_FORCE) isDimmed = true;
        else if (activeMapFilter == MapFilterMode::SUPPORT_ONLY && d->role != DroneRole::SUPPORT) isDimmed = true;
        else if (activeMapFilter == MapFilterMode::HOSTILES_ONLY) isDimmed = true;

        if (d->isDestroyed) {
            DrawText("X", dx - 4, dy - 6, 11, isDimmed ? Fade(RED, 0.3f) : RED);
            continue;
        }

        // Hover & Click Detection
        float distToMouse = Vector2Distance(mousePos, Vector2{ (float)dx, (float)dy });
        bool isHovered = (distToMouse < 12.0f);

        if (fullscreen && isHovered && mouseClicked) {
            selectedMapDroneId = d->id;
            selectedClusterId = -1;
            if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
        }

        Color dCol = (d->role == DroneRole::RECON) ? SKYBLUE : ((d->role == DroneRole::TASK_FORCE) ? ORANGE : LIME);
        if (isDimmed) dCol = Fade(dCol, 0.20f);

        int dotSize = (d->state == DroneState::IDLE_AT_BASE) ? 3 : (isHovered ? 6 : 5);
        DrawCircle(dx, dy, dotSize, dCol);

        if (fullscreen && !isDimmed) {
            DrawText(TextFormat("#%d (%.0fm)", d->id, d->position.y), dx + 7, dy - 6, 11, dCol);
        }

        // Recon Radar Detection Sweeps
        if (d->role == DroneRole::RECON && d->state != DroneState::IDLE_AT_BASE && !isDimmed) {
            DrawCircleLines(dx, dy, (int)(140 * scale), Fade(SKYBLUE, 0.35f));
            DrawCircleLines(dx, dy, (int)(70 * scale), Fade(GOLD, 0.25f));
        }

        // Selected Drone Reticle
        if (selectedMapDroneId == d->id) {
            int bSize = 12;
            DrawRectangleLines(dx - bSize, dy - bSize, bSize * 2, bSize * 2, GOLD);
            DrawCircleLines(dx, dy, bSize + 3, Fade(dCol, 0.6f));
        }
    }

    // 7. Interactive Target / Drone Analysis Cards (Left Panel)
    if (fullscreen) {
        if (selectedClusterId != -1) {
            for (const auto& c : clusters) {
                if (c.id == selectedClusterId) {
                    DrawTargetAnalysisCard2D(mapX + 20, mapY + 60, 360, mapH - 100, c, drones);
                    break;
                }
            }
        } else if (selectedMapDroneId != -1) {
            int dIdx = selectedMapDroneId - 1;
            if (dIdx >= 0 && dIdx < (int)drones.size()) {
                DrawMapDroneQuickCard2D(mapX + 20, mapY + 60, 360, mapH - 100, *drones[dIdx]);
            }
        }
    }

    // 8. Precision Tactical Mouse Reticle in Fullscreen Mode
    if (fullscreen && mousePos.x >= mapX && mousePos.x <= mapX + mapW && mousePos.y >= mapY && mousePos.y <= mapY + mapH) {
        float worldX = (mousePos.x - (float)centerX) / scale;
        float worldZ = (mousePos.y - (float)centerY) / scale;
        float mouseDist = sqrtf(worldX * worldX + worldZ * worldZ);

        DrawCircleLines((int)mousePos.x, (int)mousePos.y, 8, Fade(SKYBLUE, 0.75f));
        DrawLine((int)mousePos.x - 14, (int)mousePos.y, (int)mousePos.x + 14, (int)mousePos.y, Fade(SKYBLUE, 0.55f));
        DrawLine((int)mousePos.x, (int)mousePos.y - 14, (int)mousePos.x, (int)mousePos.y + 14, Fade(SKYBLUE, 0.55f));

        const char* coordStr = TextFormat("GRID: X:%+.0fm | Z:%+.0fm [RNG: %.0fm]", worldX, worldZ, mouseDist);
        DrawCustomText(coordStr, mousePos.x + 12, mousePos.y + 12, 10.0f, SKYBLUE);
    }
}

void FleetHUD::DrawActiveMissionsPanel2D(const std::vector<EnemyCluster>& clusters, const std::vector<std::unique_ptr<Drone>>& drones) const {
    int panelW = 340;
    int panelH = 220;
    int panelX = 20;
    int panelY = 215;

    DrawRectangle(panelX, panelY, panelW, panelH, Color{ 4, 10, 20, 245 });
    DrawRectangleLines(panelX, panelY, panelW, panelH, GOLD);
    DrawRectangleLines(panelX + 2, panelY + 2, panelW - 4, panelH - 4, Fade(GOLD, 0.35f));

    DrawCustomText("ACTIVE TASK FORCE MISSIONS", panelX + 15, panelY + 10, 14.0f, GOLD);
    DrawLine(panelX + 15, panelY + 28, panelX + panelW - 15, panelY + 28, GOLD);

    int startY = panelY + 34;
    int activeMissions = 0;

    for (const auto& c : clusters) {
        if (c.status == ClusterStatus::CLEARED) continue;
        if (c.status == ClusterStatus::REPORTED || c.status == ClusterStatus::ENGAGED) {
            activeMissions++;

            std::string tfList = "";
            std::string supList = "";
            int tfCount = 0;
            int supCount = 0;

            for (const auto& d : drones) {
                if (d->assignedClusterId == c.id && !d->isDestroyed) {
                    if (d->role == DroneRole::TASK_FORCE) {
                        tfCount++;
                        if (!tfList.empty()) tfList += ",";
                        tfList += "#" + std::to_string(d->id);
                    } else if (d->role == DroneRole::SUPPORT) {
                        supCount++;
                        if (!supList.empty()) supList += ",";
                        supList += "#" + std::to_string(d->id);
                    }
                }
            }

            Color typeCol = (c.armedCount > 0) ? Color{ 255, 60, 60, 255 } : ORANGE;
            const char* typeStr = (c.armedCount > 0) ? "ARMED" : "UNARMED";

            DrawCustomText(TextFormat("MISSION #%d - CLUSTER #%d (%s)", activeMissions, c.id, typeStr), panelX + 12, startY, 12.0f, typeCol);
            DrawCustomText(TextFormat("Units: %d | Threat Mag: %d (%d Armed, %d Unarmed)", c.remainingMagnitude, c.calculatedThreatMagnitude, c.armedCount, c.unarmedCount), panelX + 12, startY + 16, 11.0f, RAYWHITE);

            std::string assignStr = "Strike Team: ";
            if (tfCount > 0) assignStr += std::to_string(tfCount) + "x TF (" + tfList + ")";
            else assignStr += "En route / Dispatching";
            if (supCount > 0) assignStr += " | " + std::to_string(supCount) + "x SUP (" + supList + ")";

            DrawCustomText(assignStr.c_str(), panelX + 12, startY + 32, 10.0f, LIME);
            DrawLine(panelX + 12, startY + 48, panelX + panelW - 12, startY + 48, Fade(GRAY, 0.4f));

            startY += 54;
            if (activeMissions >= 3) break;
        }
    }

    if (activeMissions == 0) {
        DrawCustomText("[ NO ACTIVE STRIKE MISSIONS ]", panelX + 50, panelY + 100, 13.0f, GRAY);
        DrawCustomText("Fleet Standby / High-Alt Recon Sweep Active", panelX + 25, panelY + 122, 11.0f, DARKGRAY);
    }
}

void FleetHUD::DrawDroneInspector2D(const std::vector<std::unique_ptr<Drone>>& drones) const {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int panelW = 430;
    int panelH = 550;
    int panelX = (screenWidth - panelW) / 2;
    int panelY = (screenHeight - panelH) / 2;

    DrawRectangle(panelX, panelY, panelW, panelH, Color{ 4, 12, 24, 252 });
    DrawRectangleLines(panelX, panelY, panelW, panelH, GOLD);
    DrawRectangleLines(panelX + 2, panelY + 2, panelW - 4, panelH - 4, Fade(GOLD, 0.4f));

    DrawCustomText("INDIVIDUAL DRONE TELEMETRY INSPECTOR", panelX + 20, panelY + 15, 17.0f, GOLD);
    DrawLine(panelX + 20, panelY + 40, panelX + panelW - 20, panelY + 40, GOLD);

    int idx = inspectorDroneId - 1;
    if (idx >= 0 && idx < (int)drones.size()) {
        const auto& d = drones[idx];

        const char* roleStr = (d->role == DroneRole::RECON) ? "RECON SCOUT (4x Speed, 50m Alt, 160 HP)" : ((d->role == DroneRole::TASK_FORCE) ? "TASK FORCE STRIKE (2x Speed, Pincer, 200 HP)" : "SUPPORT LOGISTICS (Loitering Station, 300 HP)");
        Color roleCol = (d->role == DroneRole::RECON) ? SKYBLUE : ((d->role == DroneRole::TASK_FORCE) ? ORANGE : LIME);

        DrawCustomText(TextFormat("DRONE #%d - %s", d->id, roleStr), panelX + 20, panelY + 50, 15.0f, roleCol);

        const char* stateStr = "IDLE AT BASE";
        if (d->isDestroyed) stateStr = "SHOT DOWN / CRASH SITE (SAR REQUESTED)";
        else {
            switch (d->state) {
                case DroneState::PATROL: stateStr = "PATROL / GRID SWEEP"; break;
                case DroneState::SCANNING: stateStr = "SCANNING TARGET (50% RADAR ZONE)"; break;
                case DroneState::MOVING_TO_TARGET: stateStr = "MOVING TO TARGET"; break;
                case DroneState::ENGAGING: stateStr = "ENGAGING (PINCER & MISSILE SALVO)"; break;
                case DroneState::REQUESTING_SUPPORT: stateStr = "REQUESTING AERIAL SUPPORT / RENDEZVOUS"; break;
                case DroneState::RESUPPLYING_IN_AIR: stateStr = "RESUPPLYING / REPAIRING IN AIR"; break;
                case DroneState::LOITERING_IN_AREA: stateStr = "FORWARD LOITERING ORBIT (34m ALTITUDE)"; break;
                case DroneState::RETURNING_TO_BASE: stateStr = "RETURNING TO HANGAR"; break;
                default: stateStr = "IDLE AT BASE"; break;
            }
        }
        DrawCustomText(TextFormat("Status: %s", stateStr), panelX + 20, panelY + 75, 13.0f, d->isDestroyed ? RED : RAYWHITE);

        DrawCustomText(TextFormat("Position: (%.1f, %.1f, %.1f) [Alt: %.0fm]", d->position.x, d->position.y, d->position.z, d->position.y), panelX + 20, panelY + 98, 13.0f, LIGHTGRAY);
        DrawCustomText(TextFormat("Velocity: %.1f m/s", Vector3Length(d->velocity)), panelX + 20, panelY + 118, 13.0f, LIGHTGRAY);

        DrawCustomText("HULL INTEGRITY:", panelX + 20, panelY + 140, 13.0f, GREEN);
        DrawRectangle(panelX + 150, panelY + 142, 220, 13, DARKGRAY);
        DrawRectangle(panelX + 150, panelY + 142, (int)(220 * (std::max(0.0f, d->hp) / d->maxHp)), 13, d->hp < 40.0f ? RED : (d->hp < 100.0f ? YELLOW : LIME));
        DrawCustomText(TextFormat("%.0f / %.0f", std::max(0.0f, d->hp), d->maxHp), panelX + 380, panelY + 140, 11.0f, RAYWHITE);

        DrawCustomText("FUEL CAPACITY:", panelX + 20, panelY + 162, 13.0f, YELLOW);
        DrawRectangle(panelX + 150, panelY + 164, 220, 13, DARKGRAY);
        DrawRectangle(panelX + 150, panelY + 164, (int)(220 * (d->fuel / d->maxFuel)), 13, YELLOW);
        DrawCustomText(TextFormat("%.0f / %.0f", d->fuel, d->maxFuel), panelX + 380, panelY + 162, 11.0f, RAYWHITE);

        if (d->role != DroneRole::RECON) {
            DrawCustomText("AMMO CAPACITY:", panelX + 20, panelY + 184, 13.0f, RED);
            DrawRectangle(panelX + 150, panelY + 186, 220, 13, DARKGRAY);
            DrawRectangle(panelX + 150, panelY + 186, (int)(220 * (d->ammo / d->maxAmmo)), 13, RED);
            DrawCustomText(TextFormat("%.0f / %.0f", d->ammo, d->maxAmmo), panelX + 380, panelY + 184, 11.0f, RAYWHITE);
        } else {
            DrawCustomText("WEAPONS / AMMO:", panelX + 20, panelY + 184, 13.0f, GRAY);
            DrawCustomText("[ NO WEAPONS / UNARMED SCOUT ]", panelX + 150, panelY + 184, 12.0f, GRAY);
        }

        if (d->role == DroneRole::SUPPORT) {
            SupportDrone* sup = dynamic_cast<SupportDrone*>(d.get());
            if (sup) {
                DrawCustomText("CARGO FUEL:", panelX + 20, panelY + 206, 13.0f, LIME);
                DrawRectangle(panelX + 150, panelY + 208, 220, 13, DARKGRAY);
                DrawRectangle(panelX + 150, panelY + 208, (int)(220 * (sup->cargoFuel / 600.0f)), 13, LIME);
            }
        }

        // RF TELEMETRY & SNR LINK STATUS
        int rfY = (d->role == DroneRole::SUPPORT) ? 228 : 208;
        DrawCustomText("RF LINK & SNR:", panelX + 20, rfY, 13.0f, SKYBLUE);
        DrawRectangle(panelX + 150, rfY + 2, 220, 13, DARKGRAY);
        Color snrCol = d->isDestroyed ? RED : (d->snrDb < 15.0f ? RED : (d->snrDb < 25.0f ? YELLOW : LIME));
        DrawRectangle(panelX + 150, rfY + 2, (int)(220 * d->signalQuality), 13, snrCol);
        DrawCustomText(TextFormat("%.1f dB", d->snrDb), panelX + 380, rfY, 11.0f, snrCol);

        DrawCustomText(TextFormat("Range to Base: %.1fm | Latency: %.0fms | Bitrate: %.1f Mb/s", d->distanceFromBase, d->telemetryLatencyMs, d->bitrateMbps), panelX + 20, rfY + 20, 11.0f, LIGHTGRAY);

        // DEFENSIVE COUNTERMEASURES (FLARE / CHAFF) STATUS
        int cmY = rfY + 36;
        DrawCustomText("DEFENSE FLARES:", panelX + 20, cmY, 13.0f, GOLD);
        if (d->isDestroyed) {
            DrawCustomText("[ OFFLINE // CASUALTY ]", panelX + 150, cmY, 12.0f, RED);
        } else if (d->flareAlertTimer > 0.0f) {
            DrawCustomText(TextFormat("[ ACTIVE // DEPLOYED (%.1fs) ]", d->flareAlertTimer), panelX + 150, cmY, 12.0f, YELLOW);
        } else if (d->flareCooldown > 0.0f) {
            DrawCustomText(TextFormat("[ RECHARGING (%.1fs) ]", d->flareCooldown), panelX + 150, cmY, 12.0f, ORANGE);
        } else {
            DrawCustomText("[ READY // AUTO-DEFENSE (HOTKEY: X) ]", panelX + 150, cmY, 12.0f, LIME);
        }

        int logHeaderY = cmY + 24;
        DrawCustomText("INDIVIDUAL FLIGHT ACTION LOGS:", panelX + 20, logHeaderY, 14.0f, LIME);
        DrawLine(panelX + 20, logHeaderY + 18, panelX + panelW - 20, logHeaderY + 18, LIME);

        int logY = logHeaderY + 26;
        for (const auto& logMsg : d->actionLogs) {
            DrawCustomText(logMsg.c_str(), panelX + 25, logY, 11.0f, RAYWHITE);
            logY += 17;
        }
    }

    DrawCustomText("[1..9] Select Drone | [I] Close Inspector", panelX + panelW / 2 - 120, panelY + panelH - 22, 12.0f, GRAY);
}

void FleetHUD::CyclePiPTarget(int dir) {
    pipDroneTarget += dir;
    if (pipDroneTarget > 9) pipDroneTarget = 0;
    if (pipDroneTarget < 0) pipDroneTarget = 9;
}

int FleetHUD::GetEffectivePiPDroneIndex(const std::vector<std::unique_ptr<Drone>>& drones) const {
    if (drones.empty()) return 0;

    if (pipDroneTarget >= 1 && pipDroneTarget <= (int)drones.size()) {
        return pipDroneTarget - 1;
    }

    // Auto Priority Mode (0):
    // 1. Drone actively in combat / engaging
    for (int i = 0; i < (int)drones.size(); i++) {
        if (!drones[i]->isDestroyed && drones[i]->state == DroneState::ENGAGING) {
            return i;
        }
    }
    // 2. Drone requesting support / in danger
    for (int i = 0; i < (int)drones.size(); i++) {
        if (!drones[i]->isDestroyed && drones[i]->state == DroneState::REQUESTING_SUPPORT) {
            return i;
        }
    }
    // 3. Drone moving to target
    for (int i = 0; i < (int)drones.size(); i++) {
        if (!drones[i]->isDestroyed && drones[i]->state == DroneState::MOVING_TO_TARGET) {
            return i;
        }
    }
    // 4. Recon drone scanning or on patrol
    for (int i = 0; i < (int)drones.size(); i++) {
        if (!drones[i]->isDestroyed && drones[i]->role == DroneRole::RECON && drones[i]->state != DroneState::IDLE_AT_BASE) {
            return i;
        }
    }
    // 5. Inspector drone if valid
    int insIdx = inspectorDroneId - 1;
    if (insIdx >= 0 && insIdx < (int)drones.size() && !drones[insIdx]->isDestroyed) {
        return insIdx;
    }
    // 6. Default to first alive drone or 0
    for (int i = 0; i < (int)drones.size(); i++) {
        if (!drones[i]->isDestroyed) return i;
    }
    return 0;
}

void FleetHUD::DrawPiPCam2D(const std::vector<std::unique_ptr<Drone>>& drones, const std::vector<EnemyCluster>& clusters, const FleetCamera& fleetCam) const {
    if (!showPiPWindow || !pipTextureLoaded || drones.empty()) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    int targetIdx = GetEffectivePiPDroneIndex(drones);
    if (targetIdx < 0 || targetIdx >= (int)drones.size()) return;
    const auto& drone = *drones[targetIdx];

    int pipW = 340;
    int pipH = 210;
    int pipX = screenWidth - pipW - 20;
    int pipY = showTacticalMap ? (screenHeight - 200 - 20 - pipH - 12) : (screenHeight - pipH - 20);

    Color roleColor = (drone.role == DroneRole::RECON) ? SKYBLUE : ((drone.role == DroneRole::TASK_FORCE) ? ORANGE : LIME);
    Color frameBorderCol = drone.isDestroyed ? RED : (drone.snrDb < 15.0f ? RED : (drone.snrDb < 25.0f ? YELLOW : roleColor));

    // 1. Background Panel & Outer Border
    DrawRectangle(pipX, pipY, pipW, pipH, Color{ 4, 10, 20, 245 });
    DrawRectangleLines(pipX, pipY, pipW, pipH, frameBorderCol);
    DrawRectangleLines(pipX + 2, pipY + 2, pipW - 4, pipH - 4, Fade(frameBorderCol, 0.35f));

    // 2. Top Header Bar
    int headH = 24;
    DrawRectangle(pipX + 3, pipY + 3, pipW - 6, headH, Color{ 6, 14, 28, 230 });
    DrawLine(pipX + 3, pipY + headH + 3, pipX + pipW - 3, pipY + headH + 3, frameBorderCol);

    const char* modeBadge = (pipDroneTarget == 0) ? "[AUTO]" : "[LOCK]";
    const char* roleName = (drone.role == DroneRole::RECON) ? "RECON" : ((drone.role == DroneRole::TASK_FORCE) ? "STRIKE" : "SUPPORT");
    const char* titleStr = TextFormat("PiP CAM // #%d %s %s", drone.id, roleName, modeBadge);
    DrawCustomText(titleStr, pipX + 8, pipY + 7, 11.0f, roleColor);

    // Blinking REC dot
    bool recBlink = ((int)(GetTime() * 2.5f)) % 2 == 0;
    DrawCircle(pipX + pipW - 48, pipY + 14, 4, recBlink ? RED : DARKGRAY);
    DrawCustomText("LIVE", pipX + pipW - 40, pipY + 7, 10.0f, recBlink ? RED : LIGHTGRAY);

    // 3. Render 3D Video Viewport Texture (Inverted Y for OpenGL)
    int viewX = pipX + 4;
    int viewY = pipY + headH + 4;
    int viewW = pipW - 8;
    int viewH = pipH - headH - 28;

    Rectangle srcRect = { 0.0f, 0.0f, (float)pipTexture.texture.width, -(float)pipTexture.texture.height };
    Rectangle dstRect = { (float)viewX, (float)viewY, (float)viewW, (float)viewH };
    DrawTexturePro(pipTexture.texture, srcRect, dstRect, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);

    // 4. Optical Corner Brackets inside Viewport
    int bLen = 14;
    Color bracketCol = Fade(frameBorderCol, 0.65f);
    // Top-Left
    DrawLine(viewX + 4, viewY + 4, viewX + 4 + bLen, viewY + 4, bracketCol);
    DrawLine(viewX + 4, viewY + 4, viewX + 4, viewY + 4 + bLen, bracketCol);
    // Top-Right
    DrawLine(viewX + viewW - 4, viewY + 4, viewX + viewW - 4 - bLen, viewY + 4, bracketCol);
    DrawLine(viewX + viewW - 4, viewY + 4, viewX + viewW - 4, viewY + 4 + bLen, bracketCol);
    // Bottom-Left
    DrawLine(viewX + 4, viewY + viewH - 4, viewX + 4 + bLen, viewY + viewH - 4, bracketCol);
    DrawLine(viewX + 4, viewY + viewH - 4, viewX + 4, viewY + viewH - 4 - bLen, bracketCol);
    // Bottom-Right
    DrawLine(viewX + viewW - 4, viewY + viewH - 4, viewX + viewW - 4 - bLen, viewY + viewH - 4, bracketCol);
    DrawLine(viewX + viewW - 4, viewY + viewH - 4, viewX + viewW - 4, viewY + viewH - 4 - bLen, bracketCol);

    // 5. Center Boresight Reticle
    int vcx = viewX + viewW / 2;
    int vcy = viewY + viewH / 2;
    DrawLine(vcx - 16, vcy, vcx - 5, vcy, Fade(LIME, 0.6f));
    DrawLine(vcx + 5, vcy, vcx + 16, vcy, Fade(LIME, 0.6f));
    DrawLine(vcx, vcy - 16, vcx, vcy - 5, Fade(LIME, 0.6f));
    DrawLine(vcx, vcy + 5, vcx, vcy + 16, Fade(LIME, 0.6f));
    DrawCircleLines(vcx, vcy, 5, Fade(LIME, 0.4f));

    // Pitch ladder indication
    for (int p = -1; p <= 1; p++) {
        if (p == 0) continue;
        int py = vcy + p * 22;
        DrawLine(vcx - 10, py, vcx - 4, py, Fade(LIME, 0.35f));
        DrawLine(vcx + 4, py, vcx + 10, py, Fade(LIME, 0.35f));
    }

    // 6. Dynamic Signal Degradation, Glitch & Noise FX
    if (drone.isDestroyed) {
        // Destroyed: Static noise blizzard & Lost Signal warning
        for (int i = 0; i < 90; i++) {
            int rx = GetRandomValue(viewX, viewX + viewW);
            int ry = GetRandomValue(viewY, viewY + viewH);
            int rw = GetRandomValue(2, 6);
            int rh = GetRandomValue(1, 3);
            DrawRectangle(rx, ry, rw, rh, Fade(RAYWHITE, (float)GetRandomValue(30, 80) / 100.0f));
        }
        int bW = 220;
        int bH = 34;
        int bx = viewX + (viewW - bW) / 2;
        int by = viewY + (viewH - bH) / 2;
        DrawRectangle(bx, by, bW, bH, Color{ 24, 4, 4, 230 });
        DrawRectangleLines(bx, by, bW, bH, RED);
        DrawCustomText("[ NO RF CARRIER // LOSS ]", bx + 18, by + 6, 11.0f, RED);
        DrawCustomText("DOWNED AT COMBAT SECTOR", bx + 22, by + 18, 9.0f, YELLOW);
    } else {
        float noiseIntensity = 1.0f - drone.signalQuality;
        if (noiseIntensity > 0.08f) {
            int grainCount = (int)(noiseIntensity * 70.0f);
            for (int i = 0; i < grainCount; i++) {
                int rx = GetRandomValue(viewX, viewX + viewW);
                int ry = GetRandomValue(viewY, viewY + viewH);
                int rw = GetRandomValue(1, 3);
                int rh = GetRandomValue(1, 2);
                DrawRectangle(rx, ry, rw, rh, Fade(RAYWHITE, noiseIntensity * (float)GetRandomValue(25, 75) / 100.0f));
            }
            int lineCount = (int)(noiseIntensity * 5.0f);
            for (int i = 0; i < lineCount; i++) {
                int ry = GetRandomValue(viewY, viewY + viewH);
                int rh = GetRandomValue(1, 3);
                Color glitchCol = (GetRandomValue(0, 10) > 7) ? Fade(RED, noiseIntensity * 0.35f) : Fade(SKYBLUE, noiseIntensity * 0.25f);
                DrawRectangle(viewX, ry, viewW, rh, glitchCol);
            }
        }
        if (drone.flareAlertTimer > 0.0f) {
            bool cmBlink = ((int)(GetTime() * 4.0f)) % 2 == 0;
            int cmW = 200;
            int cmH = 18;
            int cmX = viewX + (viewW - cmW) / 2;
            int cmY = viewY + 7;
            DrawRectangle(cmX, cmY, cmW, cmH, Color{ 36, 20, 4, 225 });
            DrawRectangleLines(cmX, cmY, cmW, cmH, cmBlink ? GOLD : ORANGE);
            DrawCustomText("▲ FLARES / CM DEPLOYED ▲", cmX + 18, cmY + 3, 10.0f, cmBlink ? YELLOW : GOLD);
        } else if (drone.snrDb < 15.0f && ((int)(GetTime() * 3)) % 2 == 0) {
            DrawCustomText("! LOW SNR LINK !", viewX + viewW / 2 - 45, viewY + 8, 10.0f, RED);
        }
    }

    // 7. Bottom Telemetry OSD Strip
    int botY = pipY + pipH - 22;
    DrawRectangle(pipX + 3, botY, pipW - 6, 19, Color{ 5, 11, 22, 235 });
    DrawLine(pipX + 3, botY, pipX + pipW - 3, botY, Fade(frameBorderCol, 0.4f));

    float spd = Vector3Length(drone.velocity);
    const char* oadStr = TextFormat("ALT:%.0fm | SPD:%.1fm/s | SNR:%.1fdB | RNG:%.0fm", drone.position.y, spd, drone.snrDb, drone.distanceFromBase);
    DrawCustomText(oadStr, pipX + 8, botY + 4, 10.0f, RAYWHITE);
}

void FleetHUD::DrawFPVSignalOverlay2D(const Drone& activeDrone) const {
    int screenWidth = GetScreenWidth();

    int screenHeight = GetScreenHeight();

    // 1. Check if drone is destroyed
    if (activeDrone.isDestroyed) {
        // Heavy static noise
        for (int i = 0; i < 400; i++) {
            int rx = GetRandomValue(0, screenWidth);
            int ry = GetRandomValue(0, screenHeight);
            int rw = GetRandomValue(2, 8);
            int rh = GetRandomValue(1, 3);
            DrawRectangle(rx, ry, rw, rh, Fade(RAYWHITE, (float)GetRandomValue(30, 90) / 100.0f));
        }
        for (int i = 0; i < 15; i++) {
            int ry = GetRandomValue(0, screenHeight);
            int rh = GetRandomValue(2, 6);
            DrawRectangle(0, ry, screenWidth, rh, Fade(RED, 0.25f));
        }

        // Center Loss Banner
        int bannerW = 600;
        int bannerH = 80;
        int bx = (screenWidth - bannerW) / 2;
        int by = (screenHeight - bannerH) / 2;
        DrawRectangle(bx, by, bannerW, bannerH, Color{ 20, 2, 2, 240 });
        DrawRectangleLines(bx, by, bannerW, bannerH, RED);
        DrawCustomText("[ NO RF CARRIER // TRANSMISSION TERMINATED ]", bx + 30, by + 18, 18.0f, RED);
        DrawCustomText(TextFormat("DOWNED DRONE #%d AT (%.1f, %.1f) - AWAITING COMBAT SAR", activeDrone.id, activeDrone.position.x, activeDrone.position.z), bx + 30, by + 48, 13.0f, YELLOW);
        return;
    }

    // 2. FPV Military OSD Crosshairs / Corner Brackets
    int cornerMargin = 40;
    int bracketLen = 30;
    Color bracketCol = Fade(LIME, 0.4f);
    // Top-Left
    DrawLine(cornerMargin, cornerMargin, cornerMargin + bracketLen, cornerMargin, bracketCol);
    DrawLine(cornerMargin, cornerMargin, cornerMargin, cornerMargin + bracketLen, bracketCol);
    // Top-Right
    DrawLine(screenWidth - cornerMargin, cornerMargin, screenWidth - cornerMargin - bracketLen, cornerMargin, bracketCol);
    DrawLine(screenWidth - cornerMargin, cornerMargin, screenWidth - cornerMargin, cornerMargin + bracketLen, bracketCol);
    // Bottom-Left
    DrawLine(cornerMargin, screenHeight - cornerMargin, cornerMargin + bracketLen, screenHeight - cornerMargin, bracketCol);
    DrawLine(cornerMargin, screenHeight - cornerMargin, cornerMargin, screenHeight - cornerMargin - bracketLen, bracketCol);
    // Bottom-Right
    DrawLine(screenWidth - cornerMargin, screenHeight - cornerMargin, screenWidth - cornerMargin - bracketLen, screenHeight - cornerMargin, bracketCol);
    DrawLine(screenWidth - cornerMargin, screenHeight - cornerMargin, screenWidth - cornerMargin, screenHeight - cornerMargin - bracketLen, bracketCol);

    // 3. Dynamic Grain, Static Scanlines & Color Glitch based on (1.0 - signalQuality)
    float noiseIntensity = 1.0f - activeDrone.signalQuality; // 0.0 at 45dB to 1.0 at <10dB

    if (noiseIntensity > 0.08f) {
        // Digital grain particles
        int grainCount = (int)(noiseIntensity * 350.0f);
        for (int i = 0; i < grainCount; i++) {
            int rx = GetRandomValue(0, screenWidth);
            int ry = GetRandomValue(0, screenHeight);
            int rw = GetRandomValue(1, 4);
            int rh = GetRandomValue(1, 2);
            DrawRectangle(rx, ry, rw, rh, Fade(RAYWHITE, noiseIntensity * (float)GetRandomValue(20, 70) / 100.0f));
        }

        // Horizontal glitch scanlines
        int lineCount = (int)(noiseIntensity * 12.0f);
        for (int i = 0; i < lineCount; i++) {
            int ry = GetRandomValue(0, screenHeight);
            int rh = GetRandomValue(1, 4);
            Color glitchCol = (GetRandomValue(0, 10) > 7) ? Fade(RED, noiseIntensity * 0.4f) : Fade(SKYBLUE, noiseIntensity * 0.3f);
            DrawRectangle(0, ry, screenWidth, rh, glitchCol);
        }

        // Vignette darkening at edges if signal is very low
        if (noiseIntensity > 0.5f) {
            DrawRectangleLinesEx(Rectangle{ 0, 0, (float)screenWidth, (float)screenHeight }, (noiseIntensity - 0.5f) * 40.0f, Fade(BLACK, (noiseIntensity - 0.5f) * 0.7f));
        }
    }

    // 4. Military FPV Telemetry HUD Banner (Center-Top under Camera banner)
    int osdW = 500;
    int osdH = 52;
    int osdX = (screenWidth - osdW) / 2;
    int osdY = 88;

    DrawRectangle(osdX, osdY, osdW, osdH, Color{ 4, 10, 20, 210 });
    Color frameBorderCol = activeDrone.snrDb < 15.0f ? RED : (activeDrone.snrDb < 25.0f ? YELLOW : LIME);
    DrawRectangleLines(osdX, osdY, osdW, osdH, frameBorderCol);

    // Left side: RF LINK & SNR
    DrawCustomText("RF LINK:", osdX + 12, osdY + 8, 11.0f, LIGHTGRAY);
    DrawCustomText(TextFormat("%.1f dB", activeDrone.snrDb), osdX + 68, osdY + 6, 14.0f, frameBorderCol);

    // SNR Bar
    int barW = 95;
    int barH = 8;
    int barX = osdX + 135;
    int barY = osdY + 10;
    DrawRectangle(barX, barY, barW, barH, DARKGRAY);
    DrawRectangle(barX, barY, (int)(barW * activeDrone.signalQuality), barH, frameBorderCol);
    DrawRectangleLines(barX, barY, barW, barH, Fade(WHITE, 0.4f));

    // Range & Bitrate & Latency
    DrawCustomText(TextFormat("RANGE: %.0fm", activeDrone.distanceFromBase), osdX + 245, osdY + 7, 12.0f, RAYWHITE);
    DrawCustomText(TextFormat("RATE: %.1f Mb/s", activeDrone.bitrateMbps), osdX + 365, osdY + 7, 12.0f, SKYBLUE);

    DrawCustomText(TextFormat("LATENCY: %.0f ms", activeDrone.telemetryLatencyMs), osdX + 12, osdY + 30, 11.0f, LIGHTGRAY);
    const char* qualityStatus = activeDrone.snrDb >= 30.0f ? "HD VIDEO (1080p60)" : (activeDrone.snrDb >= 20.0f ? "GOOD LINK (720p30)" : (activeDrone.snrDb >= 13.0f ? "DEGRADED (480p SD)" : "CRITICAL NOISE (PACKET LOSS)"));
    DrawCustomText(qualityStatus, osdX + 135, osdY + 30, 11.0f, frameBorderCol);

    // Blinking warning banner if SNR < 15 dB
    if (activeDrone.snrDb < 15.0f) {
        if (((int)(GetTime() * 3)) % 2 == 0) {
            const char* warnStr = activeDrone.snrDb < 10.0f ? "CRITICAL: RF TELEMETRY LINK NEAR BREAKPOINT // RANGE EXCEEDED" : "WARNING: RF SIGNAL DEGRADATION DETECTED (LOW SNR)";
            float warnW = fontLoaded ? MeasureTextEx(customFont, warnStr, 13.0f, 1.0f).x : (float)MeasureText(warnStr, 13);
            DrawRectangle((int)((screenWidth - warnW) / 2.0f) - 10, osdY + osdH + 6, (int)warnW + 20, 22, Color{ 30, 4, 4, 230 });
            DrawRectangleLines((int)((screenWidth - warnW) / 2.0f) - 10, osdY + osdH + 6, (int)warnW + 20, 22, RED);
            DrawCustomText(warnStr, (screenWidth - warnW) / 2.0f, osdY + osdH + 10, 13.0f, RED);
        }
    }

    // Blinking warning banner if Flares / Countermeasures active
    if (activeDrone.flareAlertTimer > 0.0f) {
        bool flareBlink = ((int)(GetTime() * 4.0f)) % 2 == 0;
        const char* fStr = "▲ COUNTERMEASURES ACTIVE // DEFENSIVE FLARES DEPLOYED (IR DECOY) ▲";
        float fW = fontLoaded ? MeasureTextEx(customFont, fStr, 13.0f, 1.0f).x : (float)MeasureText(fStr, 13);
        int bannerY = (activeDrone.snrDb < 15.0f) ? (osdY + osdH + 32) : (osdY + osdH + 6);
        DrawRectangle((int)((screenWidth - fW) / 2.0f) - 12, bannerY, (int)fW + 24, 22, Color{ 36, 20, 4, 235 });
        DrawRectangleLines((int)((screenWidth - fW) / 2.0f) - 12, bannerY, (int)fW + 24, 22, flareBlink ? GOLD : ORANGE);
        DrawCustomText(fStr, (screenWidth - fW) / 2.0f, bannerY + 4, 13.0f, flareBlink ? YELLOW : GOLD);
    }
}

void FleetHUD::Draw2DHUD(const TelemetryContext& telemetry,
                         const std::vector<std::unique_ptr<Drone>>& drones,
                         const std::vector<EnemyCluster>& clusters,
                         const FleetCamera& fleetCam,
                         float timeScale,
                         bool isPaused) const {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    if (isMapFullscreen) {
        DrawTacticalMap2D(20, 20, screenWidth - 40, screenHeight - 40, true, drones, clusters);
        DrawCustomText("[M] Exit Fullscreen Map Mode", 40, screenHeight - 50, 15.0f, GOLD);
        return;
    }

    // 0. FLIR / NVG Scanline & Reticle Overlay (if enabled)
    if (visionMode != VisionMode::NORMAL) {
        Camera3D currentCam = fleetCam.GetCurrentCamera3D(drones);
        DrawFLIROverlay2D(currentCam, clusters, drones);
    }

    // 0.5. FPV Camera Telemetry & Dynamic Static Noise (when tracking Drone 1..9)
    int activeCam = fleetCam.GetActiveCameraMode();
    if (activeCam >= 1 && activeCam <= 9) {
        int droneIdx = activeCam - 1;
        if (droneIdx >= 0 && droneIdx < (int)drones.size()) {
            DrawFPVSignalOverlay2D(*drones[droneIdx]);
        }
    }

    // 1. Top-Left Telemetry Panel
    if (showTelemetryPanel) {
        int leftW = 340;
        int leftH = 190;
        int leftX = 20;
        int leftY = 15;

        DrawRectangle(leftX, leftY, leftW, leftH, Color{ 4, 10, 20, 240 });
        DrawRectangleLines(leftX, leftY, leftW, leftH, SKYBLUE);
        DrawRectangleLines(leftX + 2, leftY + 2, leftW - 4, leftH - 4, Fade(SKYBLUE, 0.35f));

        DrawCustomText("LOGISTICS & TELEMETRY", leftX + 15, leftY + 12, 16.0f, SKYBLUE);
        DrawLine(leftX + 15, leftY + 34, leftX + leftW - 15, leftY + 34, SKYBLUE);

        DrawCustomText(TextFormat("Total Fleet: %d Drones (Air: %d | Hangar: %d)", telemetry.totalFleetCount, telemetry.activeInMission, telemetry.dockedAtBase), leftX + 15, leftY + 42, 13.0f, RAYWHITE);
        DrawCustomText(TextFormat("Active Clusters: %d", telemetry.activeClustersCount), leftX + 15, leftY + 62, 13.0f, ORANGE);
        DrawCustomText(TextFormat("Calculated Threat Mag: %d", telemetry.knownTargetsMagnitude), leftX + 15, leftY + 82, 13.0f, YELLOW);
        DrawCustomText(TextFormat("Targets Neutralized: %d Units", telemetry.eliminatedTargetsCount), leftX + 15, leftY + 102, 13.0f, GREEN);
        DrawCustomText(TextFormat("Downed Wrecks / SAR: %d Drones", telemetry.destroyedDronesCount), leftX + 15, leftY + 122, 13.0f, RED);

        DrawCustomText("Threat Weight: Armed = 0.5 | Unarmed = 0.25", leftX + 15, leftY + 146, 10.0f, LIGHTGRAY);
        DrawCustomText("Altitude: REC 50m | TF 15-22m | SUP 34m", leftX + 15, leftY + 164, 11.0f, LIME);
    }

    // 2. Active Missions Panel
    if (showMissionsPanel) {
        DrawActiveMissionsPanel2D(clusters, drones);
    }

    // 3. Top-Right Fleet Resource Panel (9 Drones)
    if (showResourcePanel) {
        int rightW = 425;
        int rightH = 260;
        int rightX = screenWidth - rightW - 20;
        int rightY = 15;

        DrawRectangle(rightX, rightY, rightW, rightH, Color{ 4, 10, 20, 245 });
        DrawRectangleLines(rightX, rightY, rightW, rightH, ORANGE);
        DrawRectangleLines(rightX + 2, rightY + 2, rightW - 4, rightH - 4, Fade(ORANGE, 0.35f));

        DrawCustomText("FLEET SENSORS & STATUS (9 DRONES)", rightX + 15, rightY + 10, 14.0f, ORANGE);
        DrawLine(rightX + 15, rightY + 28, rightX + rightW - 15, rightY + 28, ORANGE);

        int startY = rightY + 34;
        for (int i = 0; i < (int)drones.size(); i++) {
            const auto& d = drones[i];
            const char* roleStr = (d->role == DroneRole::RECON) ? "[REC 4x]" : ((d->role == DroneRole::TASK_FORCE) ? "[TSK 2x]" : "[SUP 2x]");
            Color roleCol = (d->role == DroneRole::RECON) ? SKYBLUE : ((d->role == DroneRole::TASK_FORCE) ? ORANGE : LIME);

            const char* tag = "IDLE";
            Color tagCol = GRAY;
            if (d->isDestroyed) {
                tag = "SAR";
                tagCol = RED;
            } else {
                switch (d->state) {
                    case DroneState::PATROL: tag = "SWEEP"; tagCol = SKYBLUE; break;
                    case DroneState::SCANNING: tag = "50% RDR"; tagCol = YELLOW; break;
                    case DroneState::MOVING_TO_TARGET: tag = "MOVE"; tagCol = LIGHTGRAY; break;
                    case DroneState::ENGAGING: tag = "PINCER"; tagCol = RED; break;
                    case DroneState::REQUESTING_SUPPORT: tag = "REQ"; tagCol = GOLD; break;
                    case DroneState::RESUPPLYING_IN_AIR: tag = "SUPP"; tagCol = LIME; break;
                    case DroneState::LOITERING_IN_AREA: tag = "LOITER"; tagCol = LIME; break;
                    case DroneState::RETURNING_TO_BASE: tag = "RTB"; tagCol = ORANGE; break;
                    default: tag = "IDLE"; tagCol = GRAY; break;
                }
            }

            DrawCustomText(TextFormat("#%-2d %s", d->id, roleStr), rightX + 8, startY, 10.0f, roleCol);
            DrawCustomText(tag, rightX + 110, startY, 10.0f, tagCol);

            DrawCustomText("H", rightX + 158, startY, 10.0f, LIGHTGRAY);
            DrawRectangle(rightX + 168, startY + 2, 40, 8, DARKGRAY);
            DrawRectangle(rightX + 168, startY + 2, (int)(40 * (std::max(0.0f, d->hp) / d->maxHp)), 8, d->hp < 40.0f ? RED : (d->hp < 100.0f ? YELLOW : LIME));

            DrawCustomText("F", rightX + 214, startY, 10.0f, LIGHTGRAY);
            DrawRectangle(rightX + 224, startY + 2, 42, 8, DARKGRAY);
            DrawRectangle(rightX + 224, startY + 2, (int)(42 * (d->fuel / d->maxFuel)), 8, YELLOW);

            if (d->role != DroneRole::RECON) {
                DrawCustomText("A", rightX + 272, startY, 10.0f, LIGHTGRAY);
                DrawRectangle(rightX + 282, startY + 2, 42, 8, DARKGRAY);
                DrawRectangle(rightX + 282, startY + 2, (int)(42 * (d->ammo / d->maxAmmo)), 8, RED);
            } else {
                DrawCustomText("A -", rightX + 272, startY, 10.0f, DARKGRAY);
            }

            // S - Signal / SNR bar & value
            DrawCustomText("S", rightX + 332, startY, 10.0f, LIGHTGRAY);
            DrawRectangle(rightX + 342, startY + 2, 38, 8, DARKGRAY);
            Color sigCol = d->isDestroyed ? RED : (d->snrDb < 15.0f ? RED : (d->snrDb < 25.0f ? YELLOW : LIME));
            DrawRectangle(rightX + 342, startY + 2, (int)(38 * d->signalQuality), 8, sigCol);
            DrawCustomText(TextFormat("%.0fdB", d->snrDb), rightX + 385, startY, 9.0f, sigCol);

            startY += 22;
        }
    }

    // 4. Top-Center Camera & Time Banner
    if (showCameraBanner) {
        int bannerW = 620;
        int bannerH = 65;
        int bannerX = (screenWidth - bannerW) / 2;
        int bannerY = 15;

        DrawRectangle(bannerX, bannerY, bannerW, bannerH, Color{ 4, 10, 20, 245 });
        DrawRectangleLines(bannerX, bannerY, bannerW, bannerH, GOLD);
        DrawRectangleLines(bannerX + 2, bannerY + 2, bannerW - 4, bannerH - 4, Fade(GOLD, 0.35f));

        std::string camTitle = "CAMERA: " + fleetCam.GetActiveCameraModeName();
        if (visionMode == VisionMode::FLIR_THERMAL) {
            camTitle += " [FLIR THERMAL LWIR]";
        } else if (visionMode == VisionMode::NVG_GREEN) {
            camTitle += " [NVG GEN-3 GREEN]";
        }

        Color modeColor = (fleetCam.GetActiveCameraMode() == 0) ? SKYBLUE : ((fleetCam.GetActiveCameraMode() <= 2) ? SKYBLUE : ((fleetCam.GetActiveCameraMode() >= 8) ? LIME : ORANGE));
        if (visionMode == VisionMode::FLIR_THERMAL) modeColor = Color{ 255, 170, 40, 255 };
        else if (visionMode == VisionMode::NVG_GREEN) modeColor = Color{ 80, 255, 120, 255 };
        
        float titleW = fontLoaded ? MeasureTextEx(customFont, camTitle.c_str(), 14.0f, 1.0f).x : (float)MeasureText(camTitle.c_str(), 14);
        DrawCustomText(camTitle.c_str(), bannerX + (bannerW - titleW) / 2.0f, bannerY + 6, 14.0f, modeColor);

        const char* timeStr = isPaused ? "[P] PAUSED" : TextFormat("[P] Pause | [+] Fast (%.1fx) | [-] Slow | [K] Armed | [L] Unarmed", timeScale);
        Color timeCol = isPaused ? RED : (timeScale > 1.0f ? LIME : (timeScale < 1.0f ? YELLOW : RAYWHITE));
        float timeW = fontLoaded ? MeasureTextEx(customFont, timeStr, 12.0f, 1.0f).x : (float)MeasureText(timeStr, 12);
        DrawCustomText(timeStr, bannerX + (bannerW - timeW) / 2.0f, bannerY + 26, 12.0f, timeCol);

        const char* hintMsg = "[TAB] View | [F6] PiP | [T] FLIR | [N] NVG | [M] Map | [I] Inspector | [F1-F8] Panels";
        float hintW = fontLoaded ? MeasureTextEx(customFont, hintMsg, 11.0f, 1.0f).x : (float)MeasureText(hintMsg, 11);
        DrawCustomText(hintMsg, bannerX + (bannerW - hintW) / 2.0f, bannerY + 44, 11.0f, GOLD);
    }

    // 5. Mini Tactical Map (Bottom-Right)
    if (showTacticalMap) {
        int mapW = 200;
        int mapH = 200;
        int mapX = screenWidth - mapW - 20;
        int mapY = screenHeight - mapH - 20;
        DrawTacticalMap2D(mapX, mapY, mapW, mapH, false, drones, clusters);
    }

    // 5.5. Live Picture-in-Picture (PiP) Nose Camera Feed
    DrawPiPCam2D(drones, clusters, fleetCam);

    // 6. Tactical Operational Command Logs (Bottom-Left)
    if (showTerminalPanel) {
        int termW = 540;
        int termH = 380;
        int termX = 20;
        int termY = screenHeight - termH - 20;

        DrawRectangle(termX, termY, termW, termH, Color{ 3, 7, 14, 250 });
        DrawRectangleLines(termX, termY, termW, termH, LIME);
        DrawRectangleLines(termX + 2, termY + 2, termW - 4, termH - 4, Fade(LIME, 0.35f));

        DrawCustomText("TACTICAL OPERATIONAL COMMAND LOGS", termX + 15, termY + 10, 14.0f, LIME);
        DrawLine(termX + 15, termY + 28, termX + termW - 15, termY + 28, LIME);

        int logY = termY + 36;
        int maxLines = 19;
        int showCount = std::min(maxLines, (int)telemetry.terminalLogs.size());
        int startIndex = (int)telemetry.terminalLogs.size() - showCount;

        for (int i = startIndex; i < (int)telemetry.terminalLogs.size(); i++) {
            DrawCustomText(telemetry.terminalLogs[i].c_str(), termX + 12, logY, 11.0f, Color{ 0, 255, 120, 255 });
            logY += 17;
        }
    }

    // 7. Floating 2D Screen Overlay for Discovered Clusters
    Camera3D currentCam = fleetCam.GetCurrentCamera3D(drones);
    for (const auto& c : clusters) {
        if (c.status == ClusterStatus::CLEARED) continue;
        if (c.status == ClusterStatus::REPORTED || c.status == ClusterStatus::ENGAGED) {
            Vector3 bannerPos3D = Vector3{ c.centerPosition.x, 14.0f, c.centerPosition.z };
            Vector2 screenPos = GetWorldToScreen(bannerPos3D, currentCam);

            if (screenPos.x > 0 && screenPos.x < screenWidth && screenPos.y > 0 && screenPos.y < screenHeight) {
                Color borderCol = (c.armedCount > 0) ? Color{ 255, 40, 40, 255 } : Color{ 240, 160, 20, 255 };

                int boxW = 220;
                int boxH = 42;
                int boxX = (int)screenPos.x - boxW / 2;
                int boxY = (int)screenPos.y - boxH / 2;

                DrawRectangle(boxX, boxY, boxW, boxH, Color{ 4, 10, 20, 235 });
                DrawRectangleLines(boxX, boxY, boxW, boxH, borderCol);

                const char* typeStr = (c.armedCount > 0) ? "ARMED (Silahlı)" : "UNARMED (Silahsız)";
                DrawCustomText(TextFormat("CLUSTER #%d: %d UNITS (%s)", c.id, c.remainingMagnitude, typeStr), boxX + 8, boxY + 6, 10.0f, borderCol);
                DrawCustomText(TextFormat("Threat Mag: %d (%d Armed, %d Unarmed)", c.calculatedThreatMagnitude, c.armedCount, c.unarmedCount), boxX + 8, boxY + 22, 10.0f, RAYWHITE);
            }
        }
    }

    if (showDroneInspector) {
        DrawDroneInspector2D(drones);
    }

    if (showDebriefingPanel) {
        DrawDebriefingPanel2D(telemetry, drones);
    }
}


void FleetHUD::DrawFLIROverlay2D(const Camera3D& cam, const std::vector<EnemyCluster>& clusters, const std::vector<std::unique_ptr<Drone>>& drones) const {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    Color opticColor = (visionMode == VisionMode::FLIR_THERMAL) ? Color{ 255, 150, 40, 255 } : Color{ 60, 255, 100, 255 };
    Color dimOpticColor = Fade(opticColor, 0.45f);

    // 1. Dynamic Raster Scanlines
    float scanSpeed = 50.0f;
    float scanSpacing = 4.0f;
    float scanOffset = fmodf(GetTime() * scanSpeed, scanSpacing);
    Color scanlineCol = (visionMode == VisionMode::FLIR_THERMAL) 
        ? Fade(Color{ 100, 200, 255, 255 }, 0.045f) 
        : Fade(Color{ 80, 255, 120, 255 }, 0.055f);

    for (float y = scanOffset; y < (float)screenHeight; y += scanSpacing) {
        DrawLine(0, (int)y, screenWidth, (int)y, scanlineCol);
    }

    // 2. Optical Framing & Sensor Vignette
    DrawRectangleLinesEx(Rectangle{ 0, 0, (float)screenWidth, (float)screenHeight }, 10.0f, Fade(BLACK, 0.50f));
    DrawRectangleLines(14, 14, screenWidth - 28, screenHeight - 28, Fade(opticColor, 0.25f));

    // Corner Tactical Mils Brackets
    int cLen = 45;
    int cPad = 22;
    // Top-Left
    DrawLine(cPad, cPad, cPad + cLen, cPad, opticColor);
    DrawLine(cPad, cPad, cPad, cPad + cLen, opticColor);
    // Top-Right
    DrawLine(screenWidth - cPad, cPad, screenWidth - cPad - cLen, cPad, opticColor);
    DrawLine(screenWidth - cPad, cPad, screenWidth - cPad, cPad + cLen, opticColor);
    // Bottom-Left
    DrawLine(cPad, screenHeight - cPad, cPad + cLen, screenHeight - cPad, opticColor);
    DrawLine(cPad, screenHeight - cPad, cPad, screenHeight - cPad - cLen, opticColor);
    // Bottom-Right
    DrawLine(screenWidth - cPad, screenHeight - cPad, screenWidth - cPad - cLen, screenHeight - cPad, opticColor);
    DrawLine(screenWidth - cPad, screenHeight - cPad, screenWidth - cPad, screenHeight - cPad - cLen, opticColor);

    // 3. Central Military FLIR Reticle / Crosshair & Pitch Ladder
    int cx = screenWidth / 2;
    int cy = screenHeight / 2;

    // Crosshair with center aperture
    DrawLine(cx - 50, cy, cx - 12, cy, opticColor);
    DrawLine(cx + 12, cy, cx + 50, cy, opticColor);
    DrawLine(cx, cy - 50, cx, cy - 12, opticColor);
    DrawLine(cx, cy + 12, cx, cy + 50, opticColor);

    // Mil-Dot Range Circles
    DrawCircleLines(cx, cy, 28, dimOpticColor);
    DrawCircleLines(cx, cy, 65, Fade(opticColor, 0.25f));

    // Azimuth & Elevation Tick Marks
    DrawLine(cx, cy - 65, cx, cy - 73, opticColor);
    DrawLine(cx, cy + 65, cx, cy + 73, opticColor);
    DrawLine(cx - 65, cy, cx - 73, cy, opticColor);
    DrawLine(cx + 65, cy, cx + 73, cy, opticColor);

    // Horizon / Elevation Graduated Ladder
    for (int p = -2; p <= 2; p++) {
        if (p == 0) continue;
        int py = cy + p * 35;
        int tickW = (abs(p) == 1) ? 22 : 15;
        DrawLine(cx - tickW, py, cx - 6, py, dimOpticColor);
        DrawLine(cx + 6, py, cx + tickW, py, dimOpticColor);
        DrawText(TextFormat("%+d°", -p * 5), cx + tickW + 6, py - 4, 9, dimOpticColor);
    }

    // 4. Optical Sensor OSD Status Banner (Center Top)
    const char* modeHeader = (visionMode == VisionMode::FLIR_THERMAL)
        ? "FLIR THERMAL 8-12um LWIR // WHITE-HOT PALETTE"
        : "NVG GEN-3 INTENSIFIER // GREEN PHOSPHOR";
    const char* sensorSub = (visionMode == VisionMode::FLIR_THERMAL)
        ? "NUC: AUTO-CALIB [OK] | SENSOR: UNCOOLED MICROBOLOMETER | POLARITY: WHT-HOT | GAIN: +14.2dB"
        : "IR ILLUMINATOR: ACTIVE | AGC GAIN: HIGH | PHOSPHOR: P43 GREEN | GAIN: +22.0dB";

    int headW = fontLoaded ? (int)MeasureTextEx(customFont, modeHeader, 13.0f, 1.0f).x : MeasureText(modeHeader, 13);
    int subW = fontLoaded ? (int)MeasureTextEx(customFont, sensorSub, 10.0f, 1.0f).x : MeasureText(sensorSub, 10);
    int boxW = std::max(headW, subW) + 30;

    DrawRectangle(screenWidth / 2 - boxW / 2, 86, boxW, 36, Color{ 4, 10, 18, 220 });
    DrawRectangleLines(screenWidth / 2 - boxW / 2, 86, boxW, 36, opticColor);
    DrawCustomText(modeHeader, (float)(screenWidth / 2 - headW / 2), 90.0f, 13.0f, opticColor);
    DrawCustomText(sensorSub, (float)(screenWidth / 2 - subW / 2), 107.0f, 10.0f, RAYWHITE);

    // 5. Target Heat Acquisition Bounding Boxes & Temperature Telemetry
    int lockedTargets = 0;
    for (const auto& c : clusters) {
        if (c.status == ClusterStatus::CLEARED) continue;
        for (const auto& e : c.enemies) {
            if (!e.isAlive) continue;

            Vector2 scrPos = GetWorldToScreen(e.position, cam);
            float distToCam = Vector3Distance(cam.position, e.position);

            if (scrPos.x > 30 && scrPos.x < screenWidth - 30 && scrPos.y > 30 && scrPos.y < screenHeight - 30 && distToCam > 1.0f && distToCam < 500.0f) {
                lockedTargets++;
                int boxSize = std::clamp((int)(380.0f / distToCam * 4.0f), 18, 55);
                int bx = (int)scrPos.x - boxSize / 2;
                int by = (int)scrPos.y - boxSize / 2;

                Color tgtBorderCol = e.isArmed ? Color{ 255, 60, 40, 255 } : Color{ 255, 170, 30, 255 };
                if (visionMode == VisionMode::NVG_GREEN) {
                    tgtBorderCol = e.isArmed ? Color{ 255, 100, 100, 255 } : Color{ 80, 255, 120, 255 };
                }

                // Precision corner brackets on target
                int bLen = std::max(4, boxSize / 3);
                DrawLine(bx, by, bx + bLen, by, tgtBorderCol);
                DrawLine(bx, by, bx, by + bLen, tgtBorderCol);
                DrawLine(bx + boxSize, by, bx + boxSize - bLen, by, tgtBorderCol);
                DrawLine(bx + boxSize, by, bx + boxSize, by + bLen, tgtBorderCol);
                DrawLine(bx, by + boxSize, bx + bLen, by + boxSize, tgtBorderCol);
                DrawLine(bx, by + boxSize, bx, by + boxSize - bLen, tgtBorderCol);
                DrawLine(bx + boxSize, by + boxSize, bx + boxSize - bLen, by + boxSize, tgtBorderCol);
                DrawLine(bx + boxSize, by + boxSize, bx + boxSize, by + boxSize - bLen, tgtBorderCol);

                // Center diamond lock
                DrawCircleLines((int)scrPos.x, (int)scrPos.y, 3, tgtBorderCol);

                // Simulated Target Temperature Calculation
                float baseTemp = e.isArmed ? 342.0f : 88.0f;
                if (e.panicTimer > 0.0f) baseTemp += 115.0f;
                float simTemp = baseTemp + sinf((float)e.id * 2.3f + GetTime() * 3.0f) * 12.0f;

                const char* heatStatus = e.isArmed ? "HEAT SPIKE" : "AMBIENT";
                if (e.panicTimer > 0.0f) heatStatus = "OVERHEAT";

                const char* tempStr = TextFormat("TGT #%d-E%d [+%.1f°C %s]", c.id, e.id, simTemp, heatStatus);
                const char* distStr = TextFormat("RNG: %.0fm // %s", distToCam, e.isArmed ? "HOSTILE" : "LOGISTICS");

                DrawCustomText(tempStr, (float)(bx - 20), (float)(by - 14), 10.0f, tgtBorderCol);
                DrawCustomText(distStr, (float)(bx - 10), (float)(by + boxSize + 3), 9.0f, RAYWHITE);
            }
        }
    }
}

void FleetHUD::DrawDebriefingPanel2D(const TelemetryContext& telemetry, const std::vector<std::unique_ptr<Drone>>& drones) const {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // 1. Dim translucent screen backdrop
    DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 2, 6, 12, 215 });

    int panelW = 760;
    int panelH = 530;
    int panelX = (screenWidth - panelW) / 2;
    int panelY = (screenHeight - panelH) / 2;

    // 2. Main Glass Panel & Borders
    DrawRectangle(panelX, panelY, panelW, panelH, Color{ 4, 11, 24, 252 });
    DrawRectangleLines(panelX, panelY, panelW, panelH, GOLD);
    DrawRectangleLines(panelX + 2, panelY + 2, panelW - 4, panelH - 4, Fade(GOLD, 0.45f));
    DrawRectangleLines(panelX + 4, panelY + 4, panelW - 8, panelH - 8, Fade(SKYBLUE, 0.25f));

    // Corner Tactical Brackets
    int bLen = 22;
    DrawLine(panelX - 4, panelY - 4, panelX - 4 + bLen, panelY - 4, GOLD);
    DrawLine(panelX - 4, panelY - 4, panelX - 4, panelY - 4 + bLen, GOLD);
    DrawLine(panelX + panelW + 4, panelY - 4, panelX + panelW + 4 - bLen, panelY - 4, GOLD);
    DrawLine(panelX + panelW + 4, panelY - 4, panelX + panelW + 4, panelY - 4 + bLen, GOLD);
    DrawLine(panelX - 4, panelY + panelH + 4, panelX - 4 + bLen, panelY + panelH + 4, GOLD);
    DrawLine(panelX - 4, panelY + panelH + 4, panelX - 4, panelY + panelH + 4 - bLen, GOLD);
    DrawLine(panelX + panelW + 4, panelY + panelH + 4, panelX + panelW + 4 - bLen, panelY + panelH + 4, GOLD);
    DrawLine(panelX + panelW + 4, panelY + panelH + 4, panelX + panelW + 4, panelY + panelH + 4 - bLen, GOLD);

    // 3. Header Section
    DrawCustomText("CLASSIFIED // NATO MILITARY TACTICAL AFTER-ACTION REPORT", panelX + 25, panelY + 18, 11.0f, Fade(GOLD, 0.85f));
    DrawCustomText("TACTICAL MISSION DEBRIEFING", panelX + 25, panelY + 34, 22.0f, GOLD);
    DrawCustomText(TextFormat("OPERATION WAVE #%d // THEATER AIRSPACE SUMMARY", telemetry.operationWave), panelX + 25, panelY + 62, 13.0f, SKYBLUE);
    DrawLine(panelX + 25, panelY + 84, panelX + panelW - 25, panelY + 84, Fade(GOLD, 0.6f));

    // 4. Performance Score & Rank Evaluation Calculation
    int baseScore = 100;
    int lostPenalty = telemetry.destroyedDronesCount * 15;
    int timeMinutes = (int)(telemetry.missionTimeSecs / 60.0f);
    int timePenalty = timeMinutes * 2;
    int repairBonus = telemetry.dronesRepairedCount * 10;
    int targetBonus = telemetry.eliminatedTargetsCount * 2;
    int clusterBonus = telemetry.eliminatedClustersCount * 5;

    int totalScore = baseScore - lostPenalty - timePenalty + repairBonus + targetBonus + clusterBonus;
    totalScore = std::clamp(totalScore, 0, 100);

    const char* rankLetter = "S";
    const char* rankTitle = "PERFECT AIR SUPERIORITY";
    Color rankCol = GOLD;

    if (totalScore >= 90) {
        rankLetter = "S";
        rankTitle = "PERFECT AIR SUPERIORITY";
        rankCol = GOLD;
    } else if (totalScore >= 80) {
        rankLetter = "A";
        rankTitle = "TACTICAL COMBAT EXCELLENCE";
        rankCol = LIME;
    } else if (totalScore >= 70) {
        rankLetter = "B";
        rankTitle = "OBJECTIVE ACCOMPLISHED";
        rankCol = YELLOW;
    } else {
        rankLetter = "C";
        rankTitle = "HEAVY CASUALTIES SUSTAINED";
        rankCol = ORANGE;
    }

    // Rank Badge Box (Top Right)
    int rankBoxW = 200;
    int rankBoxH = 60;
    int rankBoxX = panelX + panelW - rankBoxW - 25;
    int rankBoxY = panelY + 16;
    DrawRectangle(rankBoxX, rankBoxY, rankBoxW, rankBoxH, Color{ 10, 22, 42, 230 });
    DrawRectangleLines(rankBoxX, rankBoxY, rankBoxW, rankBoxH, rankCol);

    DrawCircle(rankBoxX + 32, rankBoxY + 30, 22, Color{ 4, 12, 26, 255 });
    DrawCircleLines(rankBoxX + 32, rankBoxY + 30, 22, rankCol);
    DrawCustomText(rankLetter, rankBoxX + 24, rankBoxY + 16, 26.0f, rankCol);

    DrawCustomText(TextFormat("GRADE: %s (%d PTS)", rankLetter, totalScore), rankBoxX + 62, rankBoxY + 12, 13.0f, rankCol);
    DrawCustomText(rankTitle, rankBoxX + 62, rankBoxY + 32, 9.0f, RAYWHITE);

    // 5. Two Columns Matrix for Detailed Breakdown
    int colW = 345;
    int colH = 265;
    int col1X = panelX + 25;
    int col2X = panelX + panelW - colW - 25;
    int colY = panelY + 98;

    // --- COLUMN 1: COMBAT & ORDNANCE TELEMETRY ---
    DrawRectangle(col1X, colY, colW, colH, Color{ 6, 15, 30, 230 });
    DrawRectangleLines(col1X, colY, colW, colH, SKYBLUE);
    DrawCustomText("1. COMBAT & ORDNANCE METRICS", col1X + 15, colY + 12, 13.0f, SKYBLUE);
    DrawLine(col1X + 15, colY + 30, col1X + colW - 15, colY + 30, Fade(SKYBLUE, 0.4f));

    int mMin = (int)(telemetry.missionTimeSecs / 60.0f);
    int mSec = (int)(fmodf(telemetry.missionTimeSecs, 60.0f));

    int c1Y = colY + 42;
    auto DrawStatRow = [&](int x, int& y, const char* label, const char* val, Color vCol) {
        DrawCustomText(label, x + 15, y, 12.0f, LIGHTGRAY);
        DrawCustomText(val, x + colW - 15 - (fontLoaded ? MeasureTextEx(customFont, val, 12.0f, 1.0f).x : (float)MeasureText(val, 12)), y, 12.0f, vCol);
        y += 24;
    };

    DrawStatRow(col1X, c1Y, "Operation Mission Time:", TextFormat("%02d:%02d", mMin, mSec), RAYWHITE);
    DrawStatRow(col1X, c1Y, "Hostiles Eliminated:", TextFormat("%d Units", telemetry.eliminatedTargetsCount), RED);
    DrawStatRow(col1X, c1Y, "Threat Clusters Cleared:", TextFormat("%d Sectors", telemetry.eliminatedClustersCount), GOLD);
    DrawStatRow(col1X, c1Y, "Micro-Missiles Expended:", TextFormat("%d Fired", telemetry.missilesFiredCount), ORANGE);
    DrawStatRow(col1X, c1Y, "Defensive Flares Deployed:", TextFormat("%d Pods", telemetry.flaresFiredCount), YELLOW);
    DrawStatRow(col1X, c1Y, "Hostile Contact Severity:", TextFormat("Wave #%d Hostiles", telemetry.operationWave), SKYBLUE);
    DrawStatRow(col1X, c1Y, "Combat Efficiency Rating:", TextFormat("%.1f%%", (float)totalScore * 0.98f), LIME);

    // --- COLUMN 2: LOGISTICS & FLEET READINESS ---
    DrawRectangle(col2X, colY, colW, colH, Color{ 6, 15, 30, 230 });
    DrawRectangleLines(col2X, colY, colW, colH, LIME);
    DrawCustomText("2. LOGISTICS & FLEET READINESS", col2X + 15, colY + 12, 13.0f, LIME);
    DrawLine(col2X + 15, colY + 30, col2X + colW - 15, colY + 30, Fade(LIME, 0.4f));

    int activeDrones = 0;
    float avgSnr = 0.0f;
    for (const auto& d : drones) {
        if (!d->isDestroyed) activeDrones++;
        avgSnr += d->snrDb;
    }
    if (!drones.empty()) avgSnr /= (float)drones.size();

    int c2Y = colY + 42;
    DrawStatRow(col2X, c2Y, "Total Fuel Consumed:", TextFormat("%.1f Liters", telemetry.totalFuelBurned), YELLOW);
    DrawStatRow(col2X, c2Y, "Operational Fleet Strength:", TextFormat("%d / %d (%.0f%%)", activeDrones, (int)drones.size(), (float)activeDrones / (float)drones.size() * 100.0f), (activeDrones == 9) ? LIME : YELLOW);
    DrawStatRow(col2X, c2Y, "Drones Lost (Casualties):", TextFormat("%d Drones", telemetry.destroyedDronesCount), (telemetry.destroyedDronesCount == 0) ? LIME : RED);
    DrawStatRow(col2X, c2Y, "Drones Repaired / SAR Rescued:", TextFormat("%d Recoveries", telemetry.dronesRepairedCount), SKYBLUE);
    DrawStatRow(col2X, c2Y, "Average SNR Radio Quality:", TextFormat("%.1f dB", avgSnr), avgSnr > 25.0f ? LIME : YELLOW);
    DrawStatRow(col2X, c2Y, "Base Hangar Supplies:", "9999L Fuel / 9999 Ammo", LIGHTGRAY);
    DrawStatRow(col2X, c2Y, "Autonomous Fleet Status:", (activeDrones >= 7) ? "COMBAT READY" : "REINFORCEMENTS NEEDED", (activeDrones >= 7) ? LIME : ORANGE);

    // 6. Tactical Seal / Banner across Bottom-Center
    int sealW = 520;
    int sealH = 32;
    int sealX = panelX + (panelW - sealW) / 2;
    int sealY = colY + colH + 12;

    bool sealBlink = ((int)(GetTime() * 2.5f)) % 2 == 0;
    DrawRectangle(sealX, sealY, sealW, sealH, Color{ 10, 30, 20, 235 });
    DrawRectangleLines(sealX, sealY, sealW, sealH, sealBlink ? LIME : GREEN);
    DrawCustomText("★ ALL OBJECTIVES SATISFIED // AIRSPACE SECURED ★", sealX + 45, sealY + 8, 14.0f, sealBlink ? RAYWHITE : LIME);

    // 7. Footer Controls & Action Buttons
    int footY = panelY + panelH - 34;
    DrawLine(panelX + 25, footY - 8, panelX + panelW - 25, footY - 8, Fade(GOLD, 0.4f));

    DrawCustomText("[SPACE] / [ENTER] Start Next Operation Wave", panelX + 35, footY, 12.0f, GOLD);
    DrawCustomText("[F7] Close Debriefing Panel", panelX + 380, footY, 12.0f, SKYBLUE);
    DrawCustomText("[ESC] Main Menu", panelX + panelW - 145, footY, 12.0f, LIGHTGRAY);
}

} // namespace DroneFleet

