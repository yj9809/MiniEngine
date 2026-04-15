#include "Vector4.h"

#include <cmath>

namespace Engine
{
	Vector4::Vector4(float x, float y, float z, float w)
		: x(x), y(y), z(z), w(w)
	{

	}

	Vector4::Vector4(const Vector3& v, float w)
		: x(v.x), y(v.y), z(v.z), w(w)
	{

	}

	Vector4 Vector4::operator+(const Vector4& other) const
	{
		return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
	}

	Vector4 Vector4::operator-(const Vector4& other) const
	{
		return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
	}

	Vector4 Vector4::operator*(float scalar) const
	{
		return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
	}

	Vector4 Vector4::operator/(float scalar) const
	{
		return Vector4(x / scalar, y / scalar, z / scalar, w / scalar);
	}

	bool Vector4::operator==(const Vector4& other) const
	{
		return (x == other.x && y == other.y && z == other.z && w == other.w);
	}

	bool Vector4::operator!=(const Vector4& other) const
	{
		return !(*this == other);
	}

	float Vector4::Dot(const Vector4& other) const
	{
		return (x * other.x) + (y * other.y) + (z * other.z) + (w * other.w);
	}

	float Vector4::Dot(const Vector4& left, const Vector4& right)
	{
		return (left.x * right.x) + (left.y * right.y) + (left.z * right.z) + (left.w * right.w);
	}

	float Vector4::LengthSquared() const
	{
		return (x * x) + (y * y) + (z * z) + (w * w);
	}

	float Vector4::Length() const
	{
		return sqrt(LengthSquared());
	}

	Vector4 Vector4::Normalize() const
	{
		float len = Length();

		if (len == 0)
		{
			return Vector4::zero;
		}

		return Vector4(x / len, y / len, z / len, w / len);
	}

	Vector3 Vector4::ToVector3() const
	{
		return Vector3(x, y, z);
	}

	const Vector4  Vector4::zero = Vector4(0, 0, 0, 0);
	const Vector4  Vector4::one = Vector4(1, 1, 1, 1);
	const Vector4  Vector4::unitX = Vector4(1, 0, 0, 0);
	const Vector4  Vector4::unitY = Vector4(0, 1, 0, 0);
	const Vector4  Vector4::unitZ = Vector4(0, 0, 1, 0);
}