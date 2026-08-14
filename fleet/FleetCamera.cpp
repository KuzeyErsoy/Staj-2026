#include "FleetCamera.h"
#include "Drone.h"
#include "AudioManager.h"
#include <cstdio>
#include <cmath>

namespace DroneFleet {

FleetCamera::FleetCamera()
    : activeCameraMode(0),
      freeCamPos(Vector3{ 0.0f, 45.0f, 75.0f }),
      freeCamYaw(0.0f),
      freeCamPitch(-0.4f),
      trackCamYaw(0.0f),
      trackCamPitch(0.3f) {}

std::string FleetCamera::GetActiveCameraModeName() const {
    switch (activeCameraMode) {
        case 0: return "FREE COMMAND FLY CAM";
        case 1: return "TRACKING DRONE #1 [RECON 1 - 4x SPEED]";
        case 2: return "TRACKING DRONE #2 [RECON 2 - 4x SPEED]";
        case 3: return "TRACKING DRONE #3 [TASK FORCE 1 - 2x SPEED]";
        case 4: return "TRACKING DRONE #4 [TASK FORCE 2 - 2x SPEED]";
        case 5: return "TRACKING DRONE #5 [TASK FORCE 3 - 2x SPEED]";
        case 6: return "TRACKING DRONE #6 [TASK FORCE 4 - 2x SPEED]";
        case 7: return "TRACKING DRONE #7 [TASK FORCE 5 - 2x SPEED]";
        case 8: return "TRACKING DRONE #8 [SUPPORT 1 - 2x SPEED]";
        case 9: return "TRACKING DRONE #9 [SUPPORT 2 - 2x SPEED]";
        default: return "FREE CAM";
    }
}

Camera3D FleetCamera::GetCurrentCamera3D(const std::vector<std::unique_ptr<Drone>>& drones) const {
    Camera3D cam;
    cam.up = Vector3{ 0, 1, 0 };
    cam.projection = CAMERA_PERSPECTIVE;

    if (activeCameraMode == 0) {
        Vector3 forward = Vector3{ sinf(freeCamYaw) * cosf(freeCamPitch), -sinf(freeCamPitch), cosf(freeCamYaw) * cosf(freeCamPitch) };
        cam.position = freeCamPos;
        cam.target = Vector3Add(freeCamPos, forward);
        cam.fovy = 60.0f;
    } else {
        int droneIdx = activeCameraMode - 1;
        if (droneIdx >= 0 && droneIdx < (int)drones.size()) {
            const auto& targetDrone = drones[droneIdx];
            float camDist = 16.0f;
            Vector3 orbitOffset = Vector3{
                sinf(trackCamYaw) * cosf(trackCamPitch) * camDist,
                sinf(trackCamPitch) * camDist + 3.0f,
                cosf(trackCamYaw) * cosf(trackCamPitch) * camDist
            };
            cam.position = Vector3Add(targetDrone->position, orbitOffset);
            cam.target = Vector3{ targetDrone->position.x, targetDrone->position.y + 1.2f, targetDrone->position.z };
            cam.fovy = 55.0f;
        } else {
            cam.position = freeCamPos;
            cam.target = Vector3{ 0, 0, 0 };
            cam.fovy = 60.0f;
        }
    }
    return cam;
}

void FleetCamera::Update(Camera3D& camera, const std::vector<std::unique_ptr<Drone>>& drones, float dt) {
    if (IsKeyPressed(KEY_TAB)) {
        activeCameraMode = (activeCameraMode + 1) % 10;
        if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
    }
    if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0) || IsKeyPressed(KEY_C)) {
        activeCameraMode = 0;
        if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick();
    }

    if (IsKeyPressed(KEY_ONE))   { activeCameraMode = 1; if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_TWO))   { activeCameraMode = 2; if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_THREE)) { activeCameraMode = 3; if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_FOUR))  { activeCameraMode = 4; if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_FIVE))  { activeCameraMode = 5; if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_SIX))   { activeCameraMode = 6; if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_SEVEN)) { activeCameraMode = 7; if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_EIGHT)) { activeCameraMode = 8; if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }
    if (IsKeyPressed(KEY_NINE))  { activeCameraMode = 9; if (IsAudioDeviceReady()) AudioManager::Instance().PlayClick(); }

    if (activeCameraMode == 0) {
        Vector2 mouseDelta = GetMouseDelta();
        freeCamYaw -= mouseDelta.x * 0.003f;
        freeCamPitch += mouseDelta.y * 0.003f;
        if (freeCamPitch > 1.4f) freeCamPitch = 1.4f;
        if (freeCamPitch < -1.4f) freeCamPitch = -1.4f;

        float flySpeed = 50.0f * dt;
        if (IsKeyDown(KEY_LEFT_SHIFT)) flySpeed *= 2.5f;

        Vector3 forward = Vector3{ sinf(freeCamYaw) * cosf(freeCamPitch), -sinf(freeCamPitch), cosf(freeCamYaw) * cosf(freeCamPitch) };
        Vector3 right = Vector3{ cosf(freeCamYaw), 0.0f, -sinf(freeCamYaw) };

        if (IsKeyDown(KEY_W)) freeCamPos = Vector3Add(freeCamPos, Vector3Scale(forward, flySpeed));
        if (IsKeyDown(KEY_S)) freeCamPos = Vector3Subtract(freeCamPos, Vector3Scale(forward, flySpeed));

        if (IsKeyDown(KEY_D)) freeCamPos = Vector3Subtract(freeCamPos, Vector3Scale(right, flySpeed));
        if (IsKeyDown(KEY_A)) freeCamPos = Vector3Add(freeCamPos, Vector3Scale(right, flySpeed));

        if (IsKeyDown(KEY_SPACE)) freeCamPos.y += flySpeed;
        if (IsKeyDown(KEY_LEFT_CONTROL)) freeCamPos.y -= flySpeed;

        camera.position = freeCamPos;
        camera.target = Vector3Add(freeCamPos, forward);
        camera.fovy = 60.0f;
    } else {
        int droneIdx = activeCameraMode - 1;
        if (droneIdx >= 0 && droneIdx < (int)drones.size()) {
            const auto& targetDrone = drones[droneIdx];

            Vector2 mouseDelta = GetMouseDelta();
            trackCamYaw -= mouseDelta.x * 0.003f;
            trackCamPitch += mouseDelta.y * 0.003f;
            if (trackCamPitch > 1.2f) trackCamPitch = 1.2f;
            if (trackCamPitch < -0.5f) trackCamPitch = -0.5f;

            float camDist = 16.0f;
            Vector3 orbitOffset = Vector3{
                sinf(trackCamYaw) * cosf(trackCamPitch) * camDist,
                sinf(trackCamPitch) * camDist + 3.0f,
                cosf(trackCamYaw) * cosf(trackCamPitch) * camDist
            };

            Vector3 desiredCamPos = Vector3Add(targetDrone->position, orbitOffset);
            camera.position = Vector3Lerp(camera.position, desiredCamPos, dt * 7.0f);
            camera.target = Vector3Lerp(camera.target, Vector3{ targetDrone->position.x, targetDrone->position.y + 1.2f, targetDrone->position.z }, dt * 10.0f);
            camera.fovy = 55.0f;
        }
    }
}

} // namespace DroneFleet
