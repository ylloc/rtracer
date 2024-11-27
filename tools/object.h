#pragma once

#include "triangle.h"
#include "material.h"
#include "sphere.h"
#include "vector.h"

#include <algorithm>

struct Object {
    const Material* material = nullptr;
    Triangle polygon;

    const Vector* GetNormal(size_t index) const {
        return &normals[index];
    }

    const Material* GetMaterial() const {
        return material;
    }

    bool AreAnyNormalsGiven() const {
        return std::find(normals.begin(), normals.end(), Vector()) != normals.end();
    }

    std::array<Vector, 3> normals{{Vector{0, 0, 0}, Vector{0, 0, 0}, Vector{0, 0, 0}}};

    Object(const Material* material, Triangle polygon, std::array<Vector, 3> normals)
        : material(material), polygon(polygon), normals(normals) {
    }
};

struct SphereObject {
    const Material* material = nullptr;
    Sphere sphere;
};
