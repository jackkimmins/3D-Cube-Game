// MatrixSupports.hpp
#ifndef MATRIX_RENDERING_HPP
#define MATRIX_RENDERING_HPP

// 3D Vector struct
struct Vec3 {
    float x, y, z;

    // Constructor to initialise x, y, z
    Vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& v) const { return {x + v.x, y + v.y, z + v.z}; }
    Vec3 operator-(const Vec3& v) const { return {x - v.x, y - v.y, z - v.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 cross(const Vec3& v) const {
        return {
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        };
    }
    Vec3 normalize() const {
        float l = sqrtf(x * x + y * y + z * z);
        if (l < 1e-6f) // Stops any division by zero
            return {0.0f, 0.0f, 0.0f};
        return {x / l, y / l, z / l};
    }
};

// 2D Vector struct for texture coordinates
struct Vec2 {
    float u, v;

    Vec2(float u = 0.0f, float v = 0.0f) : u(u), v(v) {}

    // Subtraction operator
    Vec2 operator-(const Vec2& other) const { return Vec2(u - other.u, v - other.v); }

    // Multiplication operator with a scalar
    Vec2 operator*(float scalar) const { return Vec2(u * scalar, v * scalar); }

    // Addition operator
    Vec2 operator+(const Vec2& other) const { return Vec2(u + other.u, v + other.v); }
};

// Vertex struct combining position and texture coordinate - this needs improvement 😇
struct Vertex {
    Vec3 pos;
    Vec2 tex;

    Vertex(const Vec3& p = Vec3(), const Vec2& t = Vec2()) : pos(p), tex(t) {}
};

bool CastRay(Vec3 origin, Vec3 direction, float maxDistance, Vec3& hitBlockPosition, Vec3& hitNormal);

// 4x4 Matrix struct
struct Mat4 { float m[4][4] = {0}; };

// Multiply matrix and vector
Vec3 MultiplyMatrixVector(const Vec3& i, const Mat4& m) {
    Vec3 o;
    o.x = i.x * m.m[0][0] + i.y * m.m[1][0] + i.z * m.m[2][0] + m.m[3][0];
    o.y = i.x * m.m[0][1] + i.y * m.m[1][1] + i.z * m.m[2][1] + m.m[3][1];
    o.z = i.x * m.m[0][2] + i.y * m.m[1][2] + i.z * m.m[2][2] + m.m[3][2];
    float w = i.x * m.m[0][3] + i.y * m.m[1][3] + i.z * m.m[2][3] + m.m[3][3];
    if (w != 0.0f) o.x /= w; o.y /= w; o.z /= w;
    return o;
}

// Multiply two matrices
Mat4 MatrixMultiplyMatrix(const Mat4& m1, const Mat4& m2) {
    Mat4 matrix;
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            matrix.m[r][c] = m1.m[r][0] * m2.m[0][c] + m1.m[r][1] * m2.m[1][c] + m1.m[r][2] * m2.m[2][c] + m1.m[r][3] * m2.m[3][c];
        }
    }
    return matrix;
}

// Creates rotation matrix around Y-axis
Mat4 MatrixMakeRotationY(float angleRad) {
    Mat4 matrix;
    matrix.m[0][0] = cosf(angleRad);
    matrix.m[0][2] = sinf(angleRad);
    matrix.m[1][1] = 1.0f;
    matrix.m[2][0] = -sinf(angleRad);
    matrix.m[2][2] = cosf(angleRad);
    matrix.m[3][3] = 1.0f;
    return matrix;
}

// Creates translation matrix
Mat4 MatrixMakeTranslation(float x, float y, float z) {
    Mat4 matrix;
    matrix.m[0][0] = 1.0f;
    matrix.m[1][1] = 1.0f;
    matrix.m[2][2] = 1.0f;
    matrix.m[3][3] = 1.0f;
    matrix.m[3][0] = x;
    matrix.m[3][1] = y;
    matrix.m[3][2] = z;
    return matrix;
}

// This function creates a view matrix which is used to transform a world space point to a local space point relative to the camera
Mat4 MatrixPointAt(const Vec3& pos, const Vec3& target, const Vec3& up) {
    // Calculate new forward, up, and right vectors
    Vec3 newForward = (target - pos).normalize();
    Vec3 a = newForward * (up.dot(newForward));
    Vec3 newUp = (up - a).normalize();
    Vec3 newRight = newUp.cross(newForward);

    // Construct dimensioning and translation matrix
    Mat4 matrix;
    matrix.m[0][0] = newRight.x;  matrix.m[0][1] = newRight.y;  matrix.m[0][2] = newRight.z;  matrix.m[0][3] = 0.0f;
    matrix.m[1][0] = newUp.x;     matrix.m[1][1] = newUp.y;     matrix.m[1][2] = newUp.z;     matrix.m[1][3] = 0.0f;
    matrix.m[2][0] = newForward.x;matrix.m[2][1] = newForward.y;matrix.m[2][2] = newForward.z;matrix.m[2][3] = 0.0f;
    matrix.m[3][0] = pos.x;       matrix.m[3][1] = pos.y;       matrix.m[3][2] = pos.z;       matrix.m[3][3] = 1.0f;
    return matrix;
}

// Quick inverse of rotation/translation matrix for camera
Mat4 MatrixQuickInverse(const Mat4& m) {
    Mat4 matrix;
    matrix.m[0][0] = m.m[0][0]; matrix.m[0][1] = m.m[1][0]; matrix.m[0][2] = m.m[2][0]; matrix.m[0][3] = 0.0f;
    matrix.m[1][0] = m.m[0][1]; matrix.m[1][1] = m.m[1][1]; matrix.m[1][2] = m.m[2][1]; matrix.m[1][3] = 0.0f;
    matrix.m[2][0] = m.m[0][2]; matrix.m[2][1] = m.m[1][2]; matrix.m[2][2] = m.m[2][2]; matrix.m[2][3] = 0.0f;
    matrix.m[3][0] = -(m.m[3][0] * matrix.m[0][0] + m.m[3][1] * matrix.m[1][0] + m.m[3][2] * matrix.m[2][0]);
    matrix.m[3][1] = -(m.m[3][0] * matrix.m[0][1] + m.m[3][1] * matrix.m[1][1] + m.m[3][2] * matrix.m[2][1]);
    matrix.m[3][2] = -(m.m[3][0] * matrix.m[0][2] + m.m[3][1] * matrix.m[1][2] + m.m[3][2] * matrix.m[2][2]);
    matrix.m[3][3] = 1.0f;
    return matrix;
}

// Triangle struct
struct Triangle { Vertex v[3]; };

// Face struct
struct Face {
    Triangle tris[2];
    Vec3 normal;
};

// Mesh struct
struct Mesh { std::vector<Face> faces; };

#endif