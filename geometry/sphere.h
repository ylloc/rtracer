#pragma once

#include "vector.h"

class Sphere {
public:
    Sphere(const Vector& center, double radius) : center_(center), radius_(radius) {
    }

    const Vector& GetCenter() const {
        return center_;
    }

    double GetRadius() const {
        return radius_;
    }

    bool Contains(const Vector& point) const {
        return Length(center_ - point) <= radius_;
    }

private:
    Vector center_;
    double radius_;
};
