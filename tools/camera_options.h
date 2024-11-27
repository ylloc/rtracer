#pragma once

#include "vector.h"

#include <numbers>

struct CameraOptions {
  int screen_width;
  int screen_height;
  double fov = std::numbers::pi / 2;
  Vector look_from = {0., 0., 0.};
  Vector look_to = {0., 0., -1.};
};
