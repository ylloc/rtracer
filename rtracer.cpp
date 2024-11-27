#pragma once

#include "image.h"
#include "camera_options.h"
#include "render_options.h"
#include "geometry.h"
#include "scene.h"

#include <filesystem>

static constexpr double kEps = 1e-3;

static constexpr const double kInfDistance = 1e5;

static constexpr double kTone = 1.0 / 2.2;

RGB RGBCast(Vector color) {
  return {static_cast<int>((color[0] - kEpsilon) * 255.0),
          static_cast<int>((color[1] - kEpsilon) * 255.0),
          static_cast<int>((color[2] - kEpsilon) * 255.0)};
}

auto Sign(auto value) {
  return value < 0 ? -1 : 1;
}

// intersection ray with object = Intersection point with its material
struct IPoint {
  Intersection intersection_;
  Material const *material_{nullptr};
};

// if there is no intersection
using OIPoint = std::optional<IPoint>;

// polygon case
OIPoint GetMaybeIntersectionWithPolygon(const Ray &ray, const Object &object) {
  auto point = GetIntersection(ray, object.polygon);
  if (not point.has_value()) {
    return std::nullopt;
  }
  auto point_with_material = point.value();

  // all normals are given
  if (std::find(object.normals.begin(), object.normals.end(), Vector()) != object.normals.end()) {
    return std::optional{IPoint{point_with_material, object.material}};
  }

  auto coordinates = GetBarycentricCoords(object.polygon, point_with_material.GetPosition());

  Vector normal{};

  for (int i = 0; i < 3; ++i) {
    normal += object.normals[i] * coordinates[i];
  }
  point_with_material.SetNormal(normal);
  return std::optional{IPoint{point_with_material, object.material}};
}

std::vector<IPoint> GetAllRayIntersections(const Ray &ray, const Scene &scene) {
  std::vector<IPoint> intersections;

  for (const auto &object: scene.GetObjects()) {
    auto opt_intersection = GetMaybeIntersectionWithPolygon(ray, object);
    if (opt_intersection.has_value()) {
      intersections.push_back(opt_intersection.value());
    }
  }

  for (const auto &object: scene.GetSphereObjects()) {
    auto opt_intersection = GetIntersection(ray, object.sphere);
    if (opt_intersection.has_value()) {
      intersections.push_back(IPoint{opt_intersection.value(), object.material});
    }
  }

  return intersections;
}

OIPoint GetClosestIntersectionPoint(const Ray &ray, const Scene &scene) {
  auto all_intersections = GetAllRayIntersections(ray, scene);

  if (all_intersections.empty()) {
    return std::nullopt;
  }

  return *std::min_element(all_intersections.begin(), all_intersections.end(),
                           [](const auto &first, const auto &second) {
                             return first.intersection_.GetDistance() <
                                    second.intersection_.GetDistance();
                           });
}

void BuildTone(auto &image_pixels, int screen_width, int screen_height) {
  double total = 0;

  for (int i = 0; i < screen_width; i++) {
    for (int j = 0; j < screen_height; j++) {
      for (int k = 0; k < 3; k++) {
        total = std::max(total, std::fabs(image_pixels[i][j][k]));
      }
    }
  }

  for (int i = 0; i < screen_width; i++) {
    for (int j = 0; j < screen_height; j++) {
      for (int k = 0; k < 3; k++) {
        auto pixel = image_pixels[i][j][k];
        pixel = pixel * (pixel / (total * total) + 1.0) / (pixel + 1.0);
        image_pixels[i][j][k] = std::pow(pixel, kTone);
        if (std::isnan(image_pixels[i][j][k])) {
          image_pixels[i][j][k] = 0;
        }
      }
    }
  }
}

Vector TraceRay(const Ray &ray, Scene &scene, int depth) {
  Vector total_intensity{0, 0, 0};

  auto closest_point = GetClosestIntersectionPoint(ray, scene);

  if (!closest_point.has_value() || depth == 0) {
    return {};
  }

  auto [cl_point, m] = closest_point.value();
  auto norm = cl_point.GetNormal();
  auto material = *m;

  for (const auto &light: scene.GetLights()) {
    bool has_shadow{false};

    Ray light_ray = {light.position, Normalize(cl_point.GetPosition() - light.position)};
    auto length = Length(cl_point.GetPosition() - light.position);

    for (auto obj: scene.GetObjects()) {
      auto intersection = GetIntersection(light_ray, obj.polygon);
      has_shadow |= intersection && intersection->GetDistance() + kEps < length;
    }

    for (auto obj: scene.GetSphereObjects()) {
      auto intersection = GetIntersection(light_ray, obj.sphere);
      has_shadow |= intersection && intersection->GetDistance() + kEps < length;
    }

    if (has_shadow) {
      continue;
    }

    auto k_d =
      std::max(0.0, DotProduct(norm, Normalize(light.position - cl_point.GetPosition())));

    total_intensity += material.diffuse_color * light.intensity * k_d;

    auto calc = DotProduct(Reflect(Normalize(cl_point.GetPosition() - light.position), norm),
                           Normalize(ray.GetOrigin() - cl_point.GetPosition()));

    auto additional = std::pow(std::max(0.0, calc), material.specular_exponent);

    total_intensity += material.specular_color * light.intensity * additional;
  }

  total_intensity *= material.albedo[0];
  total_intensity += material.ambient_color + material.intensity;  // ambient

  auto point = cl_point.GetPosition();

  double al_1 = material.albedo[1], al_2 = material.albedo[2];

  auto coefficient = 1.0 / material.refraction_index;

  for (auto object: scene.GetSphereObjects()) {
    if (Length(object.sphere.GetCenter() - ray.GetOrigin()) < object.sphere.GetRadius()) {
      al_1 = 0.0, al_2 = 1.0;
      coefficient = material.refraction_index;
    }
  }

  if (al_1 != 0) {
    auto reflect_dir = Normalize(Reflect(ray.GetDirection(), norm));
    auto reflect = point + Sign(DotProduct(reflect_dir, norm)) * norm * kEps;
    total_intensity += TraceRay(Ray{reflect, reflect_dir}, scene, depth - 1) * al_1;
  }

  auto refraction = Refract(ray.GetDirection(), norm, coefficient);

  if (not refraction) {
    return total_intensity;
  }

  Vector ref = Normalize(*refraction);

  auto refract = point + Sign(DotProduct(ref, norm)) * norm * kEps;

  if (al_2 != 0.0) {
    total_intensity += TraceRay(Ray{refract, ref}, scene, depth - 1) * al_2;
  }

  return total_intensity;
}

Image Render(const std::filesystem::path &path, const CameraOptions &camera_options,
             const RenderOptions &render_options) {

  Scene scene = ReadScene(path);

  double format = (camera_options.screen_width * 1.0) / camera_options.screen_height;
  double scale = std::tan(camera_options.fov / 2);

  std::vector<Vector> tmp(camera_options.screen_height, Vector{0, 0, 0});
  std::vector<std::vector<Vector>> image_pixels(camera_options.screen_width, tmp);

  double max_among_all_rays = 0.0;

  Vector dx = {1, 0, 0};
  Vector dy = {0, 1, 0};

  for (int i = 0; i < camera_options.screen_width; i++) {
    for (int j = 0; j < camera_options.screen_height; j++) {
      double x = (2 * (i + 0.5) / camera_options.screen_width - 1) * format * scale;
      double y = (1 - 2 * (j + 0.5) / camera_options.screen_height) * scale;

      auto forward = Normalize(camera_options.look_from - camera_options.look_to);
      auto beg = camera_options.look_from;

      auto right = CrossProduct(dy, forward);
      if (right.IsZero()) {
        right = dx;
      }
      right.Normalize();

      auto up = CrossProduct(forward, right);
      if (up.IsZero()) {
        up = dx;
      }
      up.Normalize();

      Vector end = right * x + up * y - forward + beg;

      auto ray = Ray(beg, Normalize(end - beg));

      if (render_options.mode == RenderMode::kDepth) {
        double distance = kInfDistance;
        auto point = GetClosestIntersectionPoint(ray, scene);
        if (point) {
          distance = point->intersection_.GetDistance();
          max_among_all_rays = std::max(max_among_all_rays, distance);
        }
        image_pixels[i][j] = {distance, distance, distance};
      }

      if (render_options.mode == RenderMode::kNormal) {
        auto point = GetClosestIntersectionPoint(ray, scene);

        if (point) {
          image_pixels[i][j] = point->intersection_.GetNormal();
        }
      }

      if (render_options.mode == RenderMode::kFull) {
        image_pixels[i][j] = TraceRay(ray, scene, render_options.depth);
      }
    }
  }

  Image image = Image(camera_options.screen_width, camera_options.screen_height);

  /// kFull
  if (render_options.mode == RenderMode::kFull) {
    BuildTone(image_pixels, camera_options.screen_width, camera_options.screen_height);
    for (int i = 0; i < camera_options.screen_width; i++) {
      for (int j = 0; j < camera_options.screen_height; j++) {
        image.SetPixel(RGBCast(image_pixels[i][j]), j, i);
      }
    }
  }

  /// kDepth
  if (render_options.mode == RenderMode::kDepth) {
    for (int i = 0; i < camera_options.screen_width; i++) {
      for (int j = 0; j < camera_options.screen_height; j++) {
        if (image_pixels[i][j][0] == kInfDistance) {
          image_pixels[i][j] = {1.0, 1.0, 1.0};
        } else {
          image_pixels[i][j] = image_pixels[i][j] / max_among_all_rays;
        }
      }
    }
    for (int i = 0; i < camera_options.screen_width; i++) {
      for (int j = 0; j < camera_options.screen_height; j++) {
        image.SetPixel(RGBCast(image_pixels[i][j]), j, i);
      }
    }
  }

  /// kNormal
  if (render_options.mode == RenderMode::kNormal) {
    for (int i = 0; i < camera_options.screen_width; i++) {
      for (int j = 0; j < camera_options.screen_height; j++) {
        if (image_pixels[i][j].NotZero()) {
          image_pixels[i][j] *= 0.5;
          image_pixels[i][j] += 0.5;
        }
      }
    }
    for (int i = 0; i < camera_options.screen_width; i++) {
      for (int j = 0; j < camera_options.screen_height; j++) {
        image.SetPixel(RGBCast(image_pixels[i][j]), j, i);
      }
    }
  }

  return image;
}

inline std::filesystem::path GetRelativeDir(std::string_view file_path,
                                            std::string_view relative_path) {
  auto path = std::filesystem::path{file_path}.parent_path() / relative_path;
  if (path.is_absolute() && std::filesystem::is_directory(path)) {
    return path;
  } else {
    throw std::runtime_error{"Bad dir path"};
  }
}

int main() {
  CameraOptions camera_opts{.screen_width = 1000,
    .screen_height = 1000,
    .look_from = {100., 200., 150.},
    .look_to = {0., 100., 0.}};
  RenderOptions render_opts{1, RenderMode::kNormal};
  static const auto kTestsDir = GetRelativeDir(__FILE__, "tests");
  auto image = Render(kTestsDir / "CERF_Free.obj", camera_opts, render_opts);
  image.Write(kTestsDir / "result.png");
}