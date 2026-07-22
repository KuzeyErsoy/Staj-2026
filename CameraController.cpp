#include "CameraController.h"

CameraController::CameraController() {
    defaultPosition = (Vector3){ 20.0f, 20.0f, 20.0f };
    defaultTarget = (Vector3){ 0.0f, 0.0f, 0.0f };

    camera = { 0 };
    camera.position = defaultPosition;
    camera.target = defaultTarget;
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    cameraRadius = 10.0f;
    cameraYaw = 0.0f;
    cameraPitch = 0.5f;
    mouseSensitivity = 0.003f;
    isStrategicView = false;
}

void CameraController::Update(Player& player) {
    // Reset control
    if (IsKeyDown(KEY_R)) {
        Reset(player);
    }

    // Toggle Strategic View
    if (IsKeyPressed(KEY_T)) {
        isStrategicView = !isStrategicView;
        if (isStrategicView) {
            camera.position = defaultPosition;
        }
    }

    if (isStrategicView) {
        camera.target = player.position;

        if (IsKeyDown(KEY_UP)) camera.position.y += 0.5f;
        if (IsKeyDown(KEY_DOWN)) camera.position.y -= 0.5f;

        if (IsKeyDown(KEY_RIGHT)) camera.position.x += 0.5f;
        if (IsKeyDown(KEY_LEFT)) camera.position.x -= 0.5f;
        if (IsKeyDown(KEY_UP)) camera.position.z -= 0.5f;
        if (IsKeyDown(KEY_DOWN)) camera.position.z += 0.5f;
    } else {
        // Smart Cam Update (Third-Person Orbit)
        Vector2 mouseDelta = GetMouseDelta();

        cameraYaw -= mouseDelta.x * mouseSensitivity;
        cameraPitch += mouseDelta.y * mouseSensitivity;

        if (cameraPitch > 1.5f) cameraPitch = 1.5f;
        if (cameraPitch < -0.1f) cameraPitch = -0.1f;

        camera.target = (Vector3){ player.position.x, player.position.y + 1.5f, player.position.z };

        camera.position.x = camera.target.x + cameraRadius * cosf(cameraPitch) * sinf(cameraYaw);
        camera.position.y = camera.target.y + cameraRadius * sinf(cameraPitch);
        camera.position.z = camera.target.z + cameraRadius * cosf(cameraPitch) * cosf(cameraYaw);
    }
}

void CameraController::Reset(Player& player) {
    camera.position = defaultPosition;
    camera.target = defaultTarget;
    player.position = (Vector3){ 10.0f, 10.0f, 0.0f };
    player.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    isStrategicView = false;
}
