#ifndef DUMMY_GLM_HPP
#define DUMMY_GLM_HPP

#include <cmath>

namespace glm {
    struct vec2 { 
        union {
            struct { float x, y; };
            struct { float s, t; };
        };
        vec2(float v = 0.0f) : x(v), y(v) {} 
        vec2(float _x, float _y) : x(_x), y(_y) {} 
        vec2& operator+=(const vec2& v) { x += v.x; y += v.y; return *this; }
        vec2& operator-=(const vec2& v) { x -= v.x; y -= v.y; return *this; }
        vec2& operator*=(float v) { x *= v; y *= v; return *this; }
        
        vec2 operator+(const vec2& v) const { return vec2(x + v.x, y + v.y); }
        vec2 operator-(const vec2& v) const { return vec2(x - v.x, y - v.y); }
        vec2 operator*(float v) const { return vec2(x * v, y * v); }
        vec2 operator*(const vec2& v) const { return vec2(x * v.x, y * v.y); }
    };
    struct vec3 { 
        union {
            struct { float x, y, z; };
            struct { float r, g, b; };
        };
        vec3(float v = 0.0f) : x(v), y(v), z(v) {} 
        vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {} 
        vec3& operator+=(const vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
        vec3& operator-=(const vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
        vec3& operator*=(float v) { x *= v; y *= v; z *= v; return *this; }
        
        vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
        vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
        vec3 operator*(float v) const { return vec3(x * v, y * v, z * v); }
        vec3 operator*(const vec3& v) const { return vec3(x * v.x, y * v.y, z * v.z); }
    };
    struct vec4 { 
        union {
            struct { float x, y, z, w; };
            struct { float r, g, b, a; };
        };
        vec4(float v = 0.0f) : x(v), y(v), z(v), w(v) {} 
        vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {} 
        vec4& operator+=(const vec4& v) { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
        vec4& operator-=(const vec4& v) { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
        vec4& operator*=(float v) { x *= v; y *= v; z *= v; w *= v; return *this; }
        
        vec4 operator+(const vec4& v) const { return vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
        vec4 operator-(const vec4& v) const { return vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
        vec4 operator*(float v) const { return vec4(x * v, y * v, z * v, w * v); }
        vec4 operator*(const vec4& v) const { return vec4(x * v.x, y * v.y, z * v.z, w * v.w); }
    };
    struct mat4 { 
        float data[16]; 
        mat4() { for(int i=0; i<16; ++i) data[i] = (i%5==0 ? 1.0f : 0.0f); } // Identity
        mat4(float v) { for(int i=0; i<16; ++i) data[i] = (i%5==0 ? v : 0.0f); }
    };

    inline float length(const vec2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
    inline float length(const vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
    inline float length(const vec4& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w); }

    inline vec2 normalize(const vec2& v) { float l = length(v); return l > 0 ? v * (1.0f / l) : v; }
    inline vec3 normalize(const vec3& v) { float l = length(v); return l > 0 ? v * (1.0f / l) : v; }
    inline vec4 normalize(const vec4& v) { float l = length(v); return l > 0 ? v * (1.0f / l) : v; }

    inline float radians(float degrees) { return degrees * 3.14159265f / 180.0f; }

    inline float dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
    inline vec3 cross(const vec3& a, const vec3& b) {
        return vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }

    inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
        vec3 f = normalize(center - eye);
        vec3 s = normalize(cross(f, up));
        vec3 u = cross(s, f);

        mat4 Result;
        Result.data[0] = s.x;
        Result.data[4] = s.y;
        Result.data[8] = s.z;
        Result.data[1] = u.x;
        Result.data[5] = u.y;
        Result.data[9] = u.z;
        Result.data[2] = -f.x;
        Result.data[6] = -f.y;
        Result.data[10] = -f.z;
        Result.data[12] = -dot(s, eye);
        Result.data[13] = -dot(u, eye);
        Result.data[14] = dot(f, eye);
        return Result;
    }

    inline mat4 perspective(float fovy, float aspect, float zNear, float zFar) {
        float tanHalfFovy = std::tan(fovy / 2.0f);
        mat4 Result;
        for(int i=0; i<16; ++i) Result.data[i] = 0.0f;
        Result.data[0] = 1.0f / (aspect * tanHalfFovy);
        Result.data[5] = 1.0f / (tanHalfFovy);
        Result.data[10] = -(zFar + zNear) / (zFar - zNear);
        Result.data[11] = -1.0f;
        Result.data[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
        return Result;
    }

    inline mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
        mat4 Result;
        Result.data[0] = 2.0f / (right - left);
        Result.data[5] = 2.0f / (top - bottom);
        Result.data[10] = -2.0f / (zFar - zNear);
        Result.data[12] = -(right + left) / (right - left);
        Result.data[13] = -(top + bottom) / (top - bottom);
        Result.data[14] = -(zFar + zNear) / (zFar - zNear);
        return Result;
    }

    inline mat4 translate(const mat4& m, const vec3& v) {
        mat4 Result = m;
        Result.data[12] = m.data[0] * v.x + m.data[4] * v.y + m.data[8] * v.z + m.data[12];
        Result.data[13] = m.data[1] * v.x + m.data[5] * v.y + m.data[9] * v.z + m.data[13];
        Result.data[14] = m.data[2] * v.x + m.data[6] * v.y + m.data[10] * v.z + m.data[14];
        Result.data[15] = m.data[3] * v.x + m.data[7] * v.y + m.data[11] * v.z + m.data[15];
        return Result;
    }

    inline mat4 rotate(const mat4& m, float angle, const vec3& v) {
        // Very simplified rotation for now
        return m;
    }

    inline mat4 scale(const mat4& m, const vec3& v) {
        mat4 Result = m;
        Result.data[0] *= v.x; Result.data[1] *= v.x; Result.data[2] *= v.x; Result.data[3] *= v.x;
        Result.data[4] *= v.y; Result.data[5] *= v.y; Result.data[6] *= v.y; Result.data[7] *= v.y;
        Result.data[8] *= v.z; Result.data[9] *= v.z; Result.data[10] *= v.z; Result.data[11] *= v.z;
        return Result;
    }

    inline const float* value_ptr(const mat4& m) { return m.data; }
    inline const float* value_ptr(const vec2& v) { return &v.x; }
    inline const float* value_ptr(const vec3& v) { return &v.x; }
    inline const float* value_ptr(const vec4& v) { return &v.x; }
}

#endif
