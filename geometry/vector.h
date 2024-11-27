#pragma once

#include <array>
#include <cstddef>
#include <iostream>
#include <cmath>

class Vector {
public:
    Vector() = default;
    Vector(double x, double y, double z) : data_({x, y, z}) {
    }

    double& operator[](size_t ind) {
        return data_[ind];
    };

    double operator[](size_t ind) const {
        return data_[ind];
    };

    friend double Length(const Vector&);

    void Normalize() {
        if (data_ == std::array{0.0, 0.0, 0.0}) {
            return;
        }
        *this /= Length(*this);
    };

    bool NotZero() const {
        return Length(*this) != 0;
    }

    bool IsZero() const {
        return not NotZero();
    }

    Vector& operator/=(double t) {
        for (int i = 0; i < 3; ++i) {
            data_[i] /= t;
        }
        return *this;
    }

    Vector& operator+=(double t) {
        for (int i = 0; i < 3; ++i) {
            data_[i] += t;
        }
        return *this;
    }

    Vector& operator*=(double t) {
        for (int i = 0; i < 3; ++i) {
            data_[i] *= t;
        }
        return *this;
    }

    Vector& operator*=(const Vector& other) {
        for (int i = 0; i < 3; ++i) {
            data_[i] *= other[i];
        }
        return *this;
    }

    Vector& operator/=(const Vector& other) {
        for (int i = 0; i < 3; ++i) {
            data_[i] /= other[i];
        }
        return *this;
    }

    Vector& operator+=(const Vector& other) {
        for (int i = 0; i < 3; ++i) {
            data_[i] += other[i];
        }
        return *this;
    }

    double X() const {
        return data_[0];
    };

    double& X() {
        return data_[0];
    };

    double Y() const {
        return data_[1];
    };

    double& Y() {
        return data_[1];
    };

    double Z() const {
        return data_[2];
    };

    double& Z() {
        return data_[2];
    };

    bool operator==(const Vector& other) const {
        return data_ == other.data_;
    }

    auto operator<=>(const Vector& other) const {
        return data_ <=> other.data_;
    }

public:
    std::array<double, 3> data_ = {0, 0, 0};
};

double DotProduct(const Vector& a, const Vector& b) {
    double dot = 0.0;
    for (int i = 0; i < 3; ++i) {
        dot += a[i] * b[i];
    }
    return dot;
};

Vector CrossProduct(const Vector& a, const Vector& b) {
    Vector cross_product;
    cross_product.X() = a.Y() * b.Z() - a.Z() * b.Y();
    cross_product.Y() = a.Z() * b.X() - a.X() * b.Z();
    cross_product.Z() = a.X() * b.Y() - a.Y() * b.X();
    return cross_product;
};

double Length(const Vector& v) {
    double len = 0.0;
    for (int i = 0; i < 3; ++i) {
        len += v.data_[i] * v.data_[i];
    }
    return std::sqrt(len);
};

Vector operator+(const Vector& a, const Vector& b) {
    Vector sum;
    for (int i = 0; i < 3; ++i) {
        sum[i] = a[i] + b[i];
    }
    return sum;
}

Vector operator/(const Vector& a, double t) {
    Vector out;
    for (int i = 0; i < 3; ++i) {
        out[i] = a[i] / t;
    }
    return out;
}

Vector operator*(const Vector& a, const Vector& b) {
    Vector out;
    for (int i = 0; i < 3; ++i) {
        out[i] = a[i] * b[i];
    }
    return out;
}

Vector operator/(const Vector& a, const Vector& b) {
    Vector out;
    for (int i = 0; i < 3; ++i) {
        out[i] = a[i] / b[i];
    }
    return out;
}

Vector operator-(const Vector& self) {
    Vector neg;
    for (int i = 0; i < 3; ++i) {
        neg[i] = -self[i];
    }
    return neg;
}

Vector operator-(const Vector& a, const Vector& b) {
    return a + (-b);
}

Vector operator*(const Vector& a, double t) {
    Vector h;
    for (int i = 0; i < 3; ++i) {
        h[i] = a[i] * t;
    }
    return h;
}

Vector operator+(const Vector& a, double t) {
    Vector h;
    for (int i = 0; i < 3; ++i) {
        h[i] = a[i] + t;
    }
    return h;
}

Vector operator*(double t, const Vector& a) {
    return a * t;
}

Vector Normalize(Vector from) {
    from.Normalize();
    return from;
}

// debug
std::ostream& operator<<(std::ostream& out, const Vector& self) {
    out << self.X() << ' ' << self.Y() << ' ' << self.Z();
    return out;
}