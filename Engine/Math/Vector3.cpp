#include "Vector3.h"

#include <cmath>

namespace Engine
{
	Vector3::Vector3(float x, float y, float z)
		: x(x), y(y), z(z)
	{}

	Vector3::~Vector3()
	{}

	Vector3 Vector3::operator+(const Vector3& other) const
	{
		return Vector3(x + other.x, y + other.y, z + other.z);
	}

	Vector3 Vector3::operator-(const Vector3& other) const
	{
		return Vector3(x - other.x, y - other.y, z - other.z);
	}

	Vector3 Vector3::operator*(float scalar) const
	{
		return Vector3(x * scalar, y * scalar, z * scalar);
	}

	Vector3 Vector3::operator/(float scalar) const
	{
		return Vector3(x / scalar, y / scalar, z / scalar);
	}

	bool Vector3::operator==(const Vector3& other) const
	{
		return (x == other.x && y == other.y && z == other.z);
	}

	bool Vector3::operator!=(const Vector3& other) const
	{
		return !(*this == other);
	}

	float Vector3::Dot(const Vector3& other) const
	{
		return (x * other.x) + (y * other.y) + (z * other.z);
	}

	float Vector3::Dot(const Vector3& left, const Vector3& right)
	{
		return (left.x * right.x) + (left.y * right.y) + (left.z * right.z);
	}

	Vector3 Vector3::Cross(const Vector3& other) const
	{
		float newX = (y * other.z) - (z * other.y);
		float newY = (z * other.x) - (x * other.z);
		float newZ = (x * other.y) - (y * other.x);

		return Vector3(newX, newY, newZ);
	}

	Vector3 Vector3::Cross(const Vector3& left, const Vector3& right)
	{
		float newX = (left.y * right.z) - (left.z * right.y);
		float newY = (left.z * right.x) - (left.x * right.z);
		float newZ = (left.x * right.y) - (left.y * right.x);

		return Vector3(newX, newY, newZ);
	}

	float Vector3::LengthSquared() const
	{
		return (x * x) + (y * y) + (z * z);
	}

	float Vector3::Length() const
	{
		return sqrt(LengthSquared());
	}

	bool Vector3::Normalize(float minLengthSquared)
	{
		Vector3 normalized;
		if(TryNormalize(normalized, minLengthSquared))
		{
			x = normalized.x;
			y = normalized.y;
			z = normalized.z;
			return true;
		}

		return false;
	}

	bool Vector3::TryNormalize(Vector3& outResult, float minLengthSquared) const
	{
		if (!std::isfinite(minLengthSquared) || minLengthSquared < 0.0f)
		{
			outResult = Vector3::zero;
			return false;
		}

		if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
		{
			outResult = Vector3::zero;
			return false;
		}

		const double dx = static_cast<double>(x);
		const double dy = static_cast<double>(y);
		const double dz = static_cast<double>(z);

		const double lenSquared = (dx * dx) + (dy * dy) + (dz * dz);
		if (lenSquared <= minLengthSquared)
		{
			outResult = Vector3::zero;
			return false;
		}

		const double len = sqrt(lenSquared);

		const double rx = dx / len;
		const double ry = dy / len;
		const double rz = dz / len;

		if (!std::isfinite(rx) || !std::isfinite(ry) || !std::isfinite(rz))
		{
			outResult = Vector3::zero;
			return false;
		}

		outResult = Vector3(static_cast<float>(rx), static_cast<float>(ry), static_cast<float>(rz));
		return true;
	}

	const Vector3 Vector3::zero = Vector3(0, 0, 0);
	const Vector3 Vector3::one = Vector3(1, 1, 1);
	const Vector3 Vector3::unitX = Vector3(1, 0, 0);
	const Vector3 Vector3::unitY = Vector3(0, 1, 0);
	const Vector3 Vector3::unitZ = Vector3(0, 0, 1);
	const Vector3 Vector3::up = Vector3(0, 0, 1);
}
