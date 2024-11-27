#pragma once

#include "vector.h"
#include <array>
#include <cstddef>

class Triangle {
public:
    Triangle(const Vector& a, const Vector& b, const Vector& c) : vertexes_({a, b, c}) {
    }

    const Vector& operator[](size_t ind) const {
        return vertexes_[ind];
    };

    double Area() const {
        return 0.5 * Length(CrossProduct(vertexes_[1] - vertexes_[0], vertexes_[2] - vertexes_[0]));
    };

    Vector GetNormal() const {
        return CrossProduct(vertexes_[1] - vertexes_[0], vertexes_[2] - vertexes_[0]);
    }

private:
    std::array<Vector, 3> vertexes_;
};
