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

	bool Vector4::Normalize(float minLengthSquared)
	{
		Vector4 normalized;
		if (TryNormalize(normalized, minLengthSquared))
		{
			x = normalized.x;
			y = normalized.y;
			z = normalized.z;
			w = normalized.w;
			return true;
		}

		return false;
	}

	bool Vector4::TryNormalize(Vector4& outResult, float minLengthSquared) const
	{
		if (!std::isfinite(minLengthSquared) || minLengthSquared < 0.0f)
		{
			outResult = Vector4::zero;
			return false;
		}

		if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(w))
		{
			outResult = Vector4::zero;
			return false;
		}

		const double dx = static_cast<double>(x);
		const double dy = static_cast<double>(y);
		const double dz = static_cast<double>(z);
		const double dw = static_cast<double>(w);

		const double lenSquared = (dx * dx) + (dy * dy) + (dz * dz) + (dw * dw);
		if (lenSquared <= minLengthSquared)
		{
			outResult = Vector4::zero;
			return false;
		}

		const double len = sqrt(lenSquared);

		const double rx = dx / len;
		const double ry = dy / len;
		const double rz = dz / len;
		const double rw = dw / len;
		if (!std::isfinite(rx) || !std::isfinite(ry) || !std::isfinite(rz) || !std::isfinite(rw))
		{
			outResult = Vector4::zero;
			return false;
		}

		outResult = Vector4(static_cast<float>(rx), static_cast<float>(ry), static_cast<float>(rz), static_cast<float>(rw));
		return true;
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