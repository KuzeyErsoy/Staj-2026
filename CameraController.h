#pragma once
#include "Player.h"
#include "raylib.h"
#include <math.h>

class CameraController {
public:
  Camera3D camera;
  Vector3 defaultPosition;
  Vector3 defaultTarget;
  float cameraRadius;
  float cameraYaw;
  float cameraPitch;
  float mouseSensitivity;
  bool isStrategicView;

  CameraController();
  void Update(Player &player);
  void Reset(Player &player);
  Camera3D GetCamera() const { return camera; }
  float GetYaw() const { return cameraYaw; }
};
