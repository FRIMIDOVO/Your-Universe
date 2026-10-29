#pragma once
#include <cmath>
#include <ostream>
#include <istream>

struct Vec3 {
    float x, y, z;
    Vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& other) const;
    Vec3& operator+=(const Vec3& other);

    Vec3 operator-(const Vec3& other) const;
    Vec3 operator-() const;
    Vec3& operator-=(const Vec3& other);

    Vec3 operator*(float s) const;
    Vec3 operator/(float s) const;
    
    float norm() const;
    float norm_sq() const;

    float dot(const Vec3& other) const;

};

// Оператор для удобной записи в файл
inline std::ostream& operator<<(std::ostream& os, const Vec3& v);

// Оператор для удобного чтения из файла
inline std::istream& operator>>(std::istream& is, Vec3& v);