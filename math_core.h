#pragma once
#include <cmath>


struct Vector3 {
    float x, y, z;

    Vector3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

    // Vector addition
    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    // Vector subtraction
    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    // Dot Product (Useful for lighting and angles)
    float dot(const Vector3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    // Cross Product (Crucial for finding normal vectors of 3D triangles)
    Vector3 cross(const Vector3& other) const {
        return Vector3(
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        );
    }

    // Add inside Vector3 struct
    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3 normalize() const {
        float l = length();
        return l > 0.0f ? Vector3(x / l, y / l, z / l) : Vector3(0.0f, 0.0f, 0.0f);
    }

    // Multiply vector by a float (Speed)
    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
};

struct Matrix4x4 {
    float m[4][4] = { {0} };

    // Identity Matrix default constructor
    Matrix4x4() {
        m[0][0] = 1.0f; m[1][1] = 1.0f; m[2][2] = 1.0f; m[3][3] = 1.0f;
    }

    // Matrix Multiplication (Row by Column)
    Matrix4x4 operator*(const Matrix4x4& other) const {
        Matrix4x4 result;
        // Reset to true zero before adding products
        for(int i=0; i<4; i++) result.m[i][i] = 0.0f; 
        
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result.m[row][col] = m[row][0] * other.m[0][col] + 
                                     m[row][1] * other.m[1][col] + 
                                     m[row][2] * other.m[2][col] + 
                                     m[row][3] * other.m[3][col];
            }
        }
        return result;
    }

    // Creates a Translation Matrix to move objects in 3D space
    static Matrix4x4 translation(const Vector3& v) {
        Matrix4x4 result;
        result.m[0][3] = v.x;
        result.m[1][3] = v.y;
        result.m[2][3] = v.z;
        return result;
    }

    // Creates a Perspective Matrix to simulate a camera lens
    static Matrix4x4 perspective(float fovRadians, float aspect, float zNear, float zFar) {
        Matrix4x4 result;
        for(int i=0; i<4; i++) result.m[i][i] = 0.0f; // Clear identity

        float tanHalfFov = std::tan(fovRadians / 2.0f);
        
        result.m[0][0] = 1.0f / (aspect * tanHalfFov);
        result.m[1][1] = 1.0f / tanHalfFov;
        result.m[2][2] = -(zFar + zNear) / (zFar - zNear);
        result.m[2][3] = -(2.0f * zFar * zNear) / (zFar - zNear);
        result.m[3][2] = -1.0f;
        
        return result;
    }

    
    // Add inside Matrix4x4 struct
    static Matrix4x4 rotateY(float angleRadians) {
        Matrix4x4 result;
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        result.m[0][0] = c;  result.m[0][2] = s;
        result.m[2][0] = -s; result.m[2][2] = c;
        return result;
    }

    static Matrix4x4 rotateX(float angleRadians) {
        Matrix4x4 result;
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        result.m[1][1] = c;  result.m[1][2] = -s;
        result.m[2][1] = s;  result.m[2][2] = c;
        return result;
    }

    static Matrix4x4 lookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
        Vector3 f = (target - eye).normalize();
        Vector3 r = f.cross(up).normalize();
        Vector3 u = r.cross(f);

        Matrix4x4 result;
        result.m[0][0] = r.x;  result.m[0][1] = r.y;  result.m[0][2] = r.z;  result.m[0][3] = -r.dot(eye);
        result.m[1][0] = u.x;  result.m[1][1] = u.y;  result.m[1][2] = u.z;  result.m[1][3] = -u.dot(eye);
        result.m[2][0] = -f.x; result.m[2][1] = -f.y; result.m[2][2] = -f.z; result.m[2][3] = f.dot(eye);
        return result;
    }
    
    static Matrix4x4 rotateZ(float angleRadians) {
        Matrix4x4 result;
        float c = std::cos(angleRadians);
        float s = std::sin(angleRadians);
        result.m[0][0] = c;  result.m[0][1] = -s;
        result.m[1][0] = s;  result.m[1][1] = c;
        return result;
    }

    static Matrix4x4 scale(const Vector3& s) {
        Matrix4x4 result;
        for(int i=0; i<4; i++) result.m[i][i] = 0.0f; // Clear identity
        result.m[0][0] = s.x;
        result.m[1][1] = s.y;
        result.m[2][2] = s.z;
        result.m[3][3] = 1.0f;
        return result;
    }
};