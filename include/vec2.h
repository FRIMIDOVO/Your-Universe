#pragma once

#include <cmath>

struct Vec2 {
    float x, y;
    Vec2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}

    Vec2 operator+(const Vec2& other) const { return {x + other.x, y + other.y}; }
    Vec2 operator-(const Vec2& other) const { return {x - other.x, y - other.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }
    Vec2 operator-() const { return {-x, -y}; }

    Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }
        
    Vec2& operator-=(const Vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    
    float norm() const { return std::sqrt(x*x + y*y); }
    float norm_sq() const { return x*x + y*y; }
    
    void normalize() {
        float n = norm();
        if (n > 0.0001f) { x /= n; y /= n; }
    }
};