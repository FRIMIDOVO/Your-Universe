#include "vec3.h"

Vec3 Vec3::operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
Vec3& Vec3::operator+=(const Vec3& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}

Vec3 Vec3::operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
Vec3 Vec3::operator-() const { return {-x, -y, -z}; }
Vec3& Vec3::operator-=(const Vec3& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}

Vec3 Vec3::operator*(float s) const { return {x * s, y * s, z * s}; }
Vec3 Vec3::operator/(float s) const { return {x / s, y / s, z / s}; }

float Vec3::norm() const { return std::sqrt(x*x + y*y + z*z); }
float Vec3::norm_sq() const { return x*x + y*y + z*z; }

float Vec3::dot(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }


inline std::ostream& operator<<(std::ostream& os, const Vec3& v) {
    // Формат записи: "x y z"
    os << v.x << " " << v.y << " " << v.z;
    return os;
}

inline std::istream& operator>>(std::istream& is, Vec3& v) {
    // Оператор ожидает в потоке три числа через пробел
    is >> v.x >> v.y >> v.z;
    return is;
}