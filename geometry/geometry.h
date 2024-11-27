#pragma once

#include "vector.h"
#include "sphere.h"
#include "intersection.h"
#include "triangle.h"
#include "ray.h"

#include <optional>
#include <iostream>

static constexpr double kEpsilon = 1e-9;

std::optional<Intersection> GetIntersection(const Ray& ray, const Sphere& sphere) {
    auto origin = ray.GetOrigin();
    auto vector_between = sphere.GetCenter() - origin;
    auto len_of_projection_on_direction = DotProduct(vector_between, ray.GetDirection());
    auto direction = ray.GetDirection();
    auto proj = direction * len_of_projection_on_direction;
    auto d_length = Length(proj - vector_between);

    if (d_length > sphere.GetRadius()) {
        return std::nullopt;
    }

    auto delta_distance = std::sqrt(sphere.GetRadius() * sphere.GetRadius() - d_length * d_length);
    Vector position_on;

    if (len_of_projection_on_direction > 0) {
        position_on = sphere.Contains(origin) ? proj + direction * delta_distance
                                              : proj - direction * delta_distance;
    } else {
        if (sphere.Contains(origin)) {
            position_on = proj + direction * delta_distance;
        } else {
            return std::nullopt;
        }
    }

    Vector normal;
    if (Length(vector_between) > sphere.GetRadius()) {
        normal = (position_on - vector_between);
    } else {
        normal = (vector_between - position_on);
    }

    normal.Normalize();

    return Intersection(position_on + origin, normal, Length(position_on));
}

std::optional<Intersection> GetIntersection(const Ray& ray, const Triangle& triangle) {
    auto direction = ray.GetDirection();
    auto origin = ray.GetOrigin();

    auto ab = triangle[1] - triangle[0];
    auto ac = triangle[2] - triangle[0];
    auto up = CrossProduct(direction, ac);

    auto det = DotProduct(ab, up);

    // if perpendicular => direction || Lin(ab, ac), no intersection
    if (std::abs(det) < kEpsilon) {
        return std::nullopt;
    }

    auto inv_det = 1.0 / det;

    auto s = origin - triangle[0];
    auto u = inv_det * DotProduct(s, up);

    if (u < 0 || u > 1) {
        return std::nullopt;
    }

    auto t = CrossProduct(s, ab);
    auto v = inv_det * DotProduct(direction, t);

    if (v < 0 || u + v > 1) {
        return std::nullopt;
    }

    auto k = inv_det * DotProduct(ac, t);

    if (k > kEpsilon) {
        auto inter = origin + direction * k;
        auto normal = CrossProduct(ab, ac);

        // from one part of space
        if (DotProduct(normal, direction) > 0) {
            normal = -normal;
        }

        return Intersection(inter, normal, Length(origin - inter));
    }

    return std::nullopt;
};

Vector Reflect(const Vector& ray, const Vector& normal) {
    /// ray and normal are normalized by caller
    return -2.0 * DotProduct(normal, ray) * normal + ray;
}

std::optional<Vector> Refract(const Vector& ray, const Vector& normal, double eta) {
    /// ray and normal are normalized by caller
    auto normalized_ray = ray;
    normalized_ray.Normalize();

    auto c = -DotProduct(normal, normalized_ray);
    if (eta * eta * (1 - c * c) > 1) {
        return {};
    }
    return eta * normalized_ray + (eta * c - std::sqrt(1 - eta * eta * (1 - c * c))) * normal;
}

Vector GetBarycentricCoords(const Triangle& triangle, const Vector& point) {
    Vector coordinates;

    auto area = triangle.Area();

    coordinates.X() = Triangle(point, triangle[1], triangle[2]).Area() / area;
    coordinates.Y() = Triangle(point, triangle[0], triangle[2]).Area() / area;
    coordinates.Z() = Triangle(point, triangle[0], triangle[1]).Area() / area;

    return coordinates;
};
