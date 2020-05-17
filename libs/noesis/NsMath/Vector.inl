////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Math.h>
#include <NsCore/Error.h>


namespace Noesis
{

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector2::Vector2(float xx, float yy): x(xx), y(yy) {}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector2::Vector2(const float* values)
{
    NS_ASSERT(values != nullptr);
    x = values[0];
    y = values[1];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline float& Vector2::operator[](uint32_t i)
{
    NS_ASSERT(i < 2);
    return (&x)[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline const float Vector2::operator[](uint32_t i) const
{
    NS_ASSERT(i < 2);
    return (&x)[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline const float* Vector2::GetData() const
{
    return &x;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2& Vector2::operator+=(const Vector2& v)
{
    x += v.x;
    y += v.y;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2& Vector2::operator-=(const Vector2& v)
{
    x -= v.x;
    y -= v.y;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2& Vector2::operator*=(float v)
{
    x *= v;
    y *= v;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2& Vector2::operator/=(float v)
{
    x /= v;
    y /= v;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Vector2::operator==(const Vector2& v) const
{
    return x == v.x && y == v.y;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Vector2::operator!=(const Vector2& v) const
{
    return !(operator==(v));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector2::Zero()
{
    return Vector2(0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector2::XAxis()
{
    return Vector2(1.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector2::YAxis()
{
    return Vector2(0.0f, 1.0f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float Length(const Vector2& v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float LengthSquared(const Vector2& v)
{
    return v.x * v.x + v.y * v.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector2 Normalize(const Vector2& v)
{
    NS_ASSERT(Length(v) > FLT_EPSILON);
    float len = Length(v);
    return Vector2(v.x / len, v.y / len);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector2 PerpendicularCCW(const Vector2& v)
{
    return Vector2(-v.y, v.x);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector2 PerpendicularCW(const Vector2& v)
{
    return Vector2(v.y, -v.x);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector2 Perpendicular(const Vector2& v, bool cw)
{
    return cw ? Vector2(v.y, -v.x) : Vector2(-v.y, v.x);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float Dot(const Vector2& v0, const Vector2& v1)
{
    return v0.x * v1.x + v0.y * v1.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float PerpDot(const Vector2& v0, const Vector2& v1)
{
    return v0.x * v1.y - v0.y * v1.x;
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector2 Lerp(const Vector2& v0, const Vector2& v1, float t)
{
    return Vector2(Noesis::Lerp(v0.x, v1.x, t), Noesis::Lerp(v0.y, v1.y, t));
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float SignedAngle(const Vector2& v0, const Vector2& v1)
{
    return atan2f(PerpDot(v0, v1), Dot(v0, v1));
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector2 operator+(const Vector2& v)
{
    return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector2 operator-(const Vector2& v)
{
    return Vector2(-v.x, -v.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector2 operator+(const Vector2& v0, const Vector2& v1)
{
    return Vector2(v0.x + v1.x, v0.y + v1.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector2 operator-(const Vector2& v0, const Vector2& v1)
{
    return Vector2(v0.x - v1.x, v0.y - v1.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector2 operator*(const Vector2& v, float f)
{
    return Vector2(v.x * f, v.y * f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector2 operator*(float f, const Vector2& v)
{
    return Vector2(v.x * f, v.y * f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector2 operator/(const Vector2& v, float f)
{
    return Vector2(v.x / f, v.y / f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3::Vector3(float xx, float yy, float zz): x(xx), y(yy), z(zz) {}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3::Vector3(const float* values)
{
    NS_ASSERT(values != nullptr);
    x = values[0];
    y = values[1];
    z = values[2];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3::Vector3(const Vector2& v, float zz) : x(v.x), y(v.y), z(zz) {}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline float& Vector3::operator[](uint32_t i)
{
    NS_ASSERT(i < 3);
    return (&x)[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline const float Vector3::operator[](uint32_t i) const
{
    NS_ASSERT(i < 3);
    return (&x)[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline const float* Vector3::GetData() const
{
    return &x;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector3::XY() const
{
    return Vector2(x, y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector3::XZ() const
{
    return Vector2(x, z);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector3::YZ() const
{
    return Vector2(y, z);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3& Vector3::operator+=(const Vector3& v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3& Vector3::operator-=(const Vector3& v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3& Vector3::operator*=(float v)
{
    x *= v;
    y *= v;
    z *= v;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3& Vector3::operator/=(float v)
{
    x /= v;
    y /= v;
    z /= v;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Vector3::operator==(const Vector3& v) const
{
    return x == v.x && y == v.y && z == v.z;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Vector3::operator!=(const Vector3& v) const
{
    return !(operator==(v));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3 Vector3::Zero()
{
    return Vector3(0.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3 Vector3::XAxis()
{
    return Vector3(1.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3 Vector3::YAxis()
{
    return Vector3(0.0f, 1.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3 Vector3::ZAxis()
{
    return Vector3(0.0f, 0.0f, 1.0f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float Length(const Vector3& v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float LengthSquared(const Vector3& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector3 Normalize(const Vector3& v)
{
    NS_ASSERT(Length(v) > FLT_EPSILON);
    float lenght = Length(v);
    return Vector3(v.x / lenght, v.y / lenght, v.z / lenght);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector2 Project(const Vector3& v)
{
    NS_ASSERT(v.z > FLT_EPSILON);
    return Vector2(v.x / v.z, v.y / v.z);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float Dot(const Vector3& v0, const Vector3& v1)
{
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector3 Cross(const Vector3& v0, const Vector3& v1)
{
    return Vector3(v0.y * v1.z - v0.z * v1.y, v0.z * v1.x - v0.x * v1.z, v0.x * v1.y - v0.y * v1.x);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector3 operator+(const Vector3& v)
{
    return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector3 operator-(const Vector3& v)
{
    return Vector3(-v.x, -v.y, -v.z);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector3 operator+(const Vector3& v0, const Vector3& v1)
{
    return Vector3(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector3 operator-(const Vector3& v0, const Vector3& v1)
{
    return Vector3(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector3 operator*(const Vector3& v, float f)
{
    return Vector3(v.x * f, v.y * f, v.z * f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector3 operator*(float f, const Vector3& v)
{
    return Vector3(v.x * f, v.y * f, v.z * f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector3 operator/(const Vector3& v, float f)
{
    return Vector3(v.x / f, v.y / f, v.z / f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector4::Vector4(float xx, float yy, float zz, float ww): x(xx), y(yy), z(zz), w(ww) {}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector4::Vector4(const float* values)
{
    NS_ASSERT(values != nullptr);
    x = values[0];
    y = values[1];
    z = values[2];
    w = values[3];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4::Vector4(const Vector3& v, float ww): x(v.x), y(v.y), z(v.z), w(ww) {}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4::Vector4(const Vector2& v, float zz, float ww): x(v.x), y(v.y), z(zz), w(ww) {}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline float& Vector4::operator[](uint32_t i)
{
    NS_ASSERT(i < 4);
    return (&x)[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline const float Vector4::operator[](uint32_t i) const
{
    NS_ASSERT(i < 4);
    return (&x)[i];
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline const float* Vector4::GetData() const
{
    return &x;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector4::XY() const
{
    return Vector2(x, y);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector4::XZ() const
{
    return Vector2(x, z);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector4::XW() const
{
    return Vector2(x, w);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector4::YZ() const
{
    return Vector2(y, z);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector2 Vector4::YW() const
{
    return Vector2(y, w);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3 Vector4::XYZ() const
{
    return Vector3(x, y, z);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector3 Vector4::XYW() const
{
    return Vector3(x, y, w);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4& Vector4::operator+=(const Vector4& v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4& Vector4::operator-=(const Vector4& v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    w -= v.w;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4& Vector4::operator*=(float v)
{
    x *= v;
    y *= v;
    z *= v;
    w *= v;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4& Vector4::operator/=(float v)
{
    x /= v;
    y /= v;
    z /= v;
    w /= v;
    return *this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Vector4::operator==(const Vector4& v) const
{
    return x == v.x && y == v.y && z == v.z && w == v.w;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Vector4::operator!=(const Vector4& v) const
{
    return !(operator==(v));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4 Vector4::Zero()
{
    return Vector4(0.0f, 0.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4 Vector4::XAxis()
{
    return Vector4(1.0f, 0.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4 Vector4::YAxis()
{
    return Vector4(0.0f, 1.0f, 0.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4 Vector4::ZAxis()
{
    return Vector4(0.0f, 0.0f, 1.0f, 0.0f);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline Vector4 Vector4::WAxis()
{
    return Vector4(0.0f, 0.0f, 0.0f, 1.0f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float Length(const Vector4& v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float LengthSquared(const Vector4& v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector4 Normalize(const Vector4& v)
{
    NS_ASSERT(Length(v) > FLT_EPSILON);
    float length = Length(v);
    return Vector4(v.x / length, v.y / length, v.z / length, v.w / length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline Vector3 Project(const Vector4& v)
{
    NS_ASSERT(v.w > FLT_EPSILON);
    return Vector3(v.x / v.w, v.y / v.w, v.z / v.w);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline float Dot(const Vector4& v0, const Vector4& v1)
{
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector4 operator+(const Vector4& v)
{
    return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector4 operator-(const Vector4& v)
{
    return Vector4(-v.x, -v.y, -v.z, -v.w);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector4 operator+(const Vector4& v0, const Vector4& v1)
{
    return Vector4(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z, v0.w + v1.w);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector4 operator-(const Vector4& v0, const Vector4& v1)
{
    return Vector4(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z, v0.w - v1.w);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector4 operator*(const Vector4& v, float f)
{
    return Vector4(v.x * f, v.y * f, v.z * f, v.w * f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector4 operator*(float f, const Vector4& v)
{
    return Vector4(v.x * f, v.y * f, v.z * f, v.w * f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////// 
inline const Vector4 operator/(const Vector4& v, float f)
{
    return Vector4(v.x / f, v.y / f, v.z / f, v.w / f);
}

}