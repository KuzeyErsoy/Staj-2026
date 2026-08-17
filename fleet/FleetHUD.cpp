#include "FleetHUD.h"
#include "Drone.h"
#include "FleetCamera.h"
#include "ClusterManager.h"
#include <algorithm>
#include <cstdio>

namespace DroneFleet {

FleetHUD::FleetHUD() : fontLoaded(false) {
    Init();
}

FleetHUD::~FleetHUD() {
    if (fontLoaded) {
        UnloadFont(customFont);
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

void FleetHUD::DrawTacticalMap2D(int mapX, int mapY, int mapW, int mapH, bool fullscreen, const std::vector<std::unique_ptr<Drone>>& drones, const std::vector<EnemyCluster>& clusters) const {
    DrawRectangle(mapX, mapY, mapW, mapH, Color{ 3, 8, 16, 245 });
    DrawRectangleLines(mapX, mapY, mapW, mapH, SKYBLUE);
    DrawRectangleLines(mapX + 2, mapY + 2, mapW - 4, mapH - 4, Fade(SKYBLUE, 0.35f));

    DrawCustomText(fullscreen ? "TACTICAL RADAR ARENA MAP - 1000x1000m GRID (FULLSCREEN)" : "TACTICAL MAP",
                   mapX + 15, mapY + 10, fullscreen ? 18.0f : 14.0f, SKYBLUE);

    float scale = (float)mapW / 1000.0f;
    int centerX = mapX + mapW / 2;
    int centerY = mapY + mapH / 2;

    DrawCircleLines(centerX, centerY, (int)(200 * scale), Fade(SKYBLUE, 0.25f));
    DrawCircleLines(centerX, centerY, (int)(400 * scale), Fade(SKYBLUE, 0.25f));
    DrawLine(mapX, centerY, mapX + mapW, centerY, Fade(SKYBLUE, 0.2f));
    DrawLine(centerX, mapY, centerX, mapY + mapH, Fade(SKYBLUE, 0.2f));

    DrawRectangleLines(centerX - 6, centerY - 6, 12, 12, SKYBLUE);
    DrawCustomText("BASE", centerX - 12, centerY - 18, 10.0f, SKYBLUE);

    for (const auto& c : clusters) {
        if (c.status == ClusterStatus::CLEARED) continue;

        int cx = centerX + (int)(c.centerPosition.x * scale);
        int cy = centerY + (int)(c.centerPosition.z * scale);

        Color cCol = RED;
        if (c.status == ClusterStatus::UNDETECTED) cCol = YELLOW;
        else if (c.status == ClusterStatus::REPORTED) cCol = ORANGE;

        DrawCircle(cx, cy, 5, cCol);
        if (fullscreen) {
            DrawText(TextFormat("#%d (Mag:%d)", c.id, c.calculatedThreatMagnitude), cx + 7, cy - 5, 11, RAYWHITE);
        }
    }

    for (const auto& d : drones) {
        int dx = centerX + (int)(d->position.x * scale);
        int dy = centerY + (int)(d->position.z * scale);

        if (d->isDestroyed) {
            DrawText("X", dx - 3, dy - 5, 10, RED);
            continue;
        }

        Color dCol = (d->role == DroneRole::RECON) ? SKYBLUE : ((d->role == DroneRole::TASK_FORCE) ? ORANGE : LIME);
        DrawCircle(dx, dy, (d->state == DroneState::IDLE_AT_BASE) ? 3 : 5, dCol);

        if (fullscreen) {
            DrawText(TextFormat("#%d", d->id), dx + 6, dy - 6, 11, dCol);
        }

        if (d->role == DroneRole::RECON && d->state != DroneState::IDLE_AT_BASE) {
            DrawCircleLines(dx, dy, (int)(140 * scale), Fade(SKYBLUE, 0.35f));
            DrawCircleLines(dx, dy, (int)(70 * scale), Fade(GOLD, 0.25f));
        }
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

        int logHeaderY = rfY + 44;
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

    // 0. FPV Camera Telemetry & Dynamic Static Noise (when tracking Drone 1..9)
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
        Color modeColor = (fleetCam.GetActiveCameraMode() == 0) ? SKYBLUE : ((fleetCam.GetActiveCameraMode() <= 2) ? SKYBLUE : ((fleetCam.GetActiveCameraMode() >= 8) ? LIME : ORANGE));
        
        float titleW = fontLoaded ? MeasureTextEx(customFont, camTitle.c_str(), 14.0f, 1.0f).x : (float)MeasureText(camTitle.c_str(), 14);
        DrawCustomText(camTitle.c_str(), bannerX + (bannerW - titleW) / 2.0f, bannerY + 6, 14.0f, modeColor);

        const char* timeStr = isPaused ? "[P] PAUSED" : TextFormat("[P] Pause | [+] Fast (%.1fx) | [-] Slow | [K] Armed | [L] Unarmed", timeScale);
        Color timeCol = isPaused ? RED : (timeScale > 1.0f ? LIME : (timeScale < 1.0f ? YELLOW : RAYWHITE));
        float timeW = fontLoaded ? MeasureTextEx(customFont, timeStr, 12.0f, 1.0f).x : (float)MeasureText(timeStr, 12);
        DrawCustomText(timeStr, bannerX + (bannerW - timeW) / 2.0f, bannerY + 26, 12.0f, timeCol);

        const char* hintMsg = "[TAB] View | [M] Map | [I] Telemetry | [F1-F8] Panels";
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
}


} // namespace DroneFleet
