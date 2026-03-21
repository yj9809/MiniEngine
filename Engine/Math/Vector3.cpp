#include "Vector3.h"

namespace Engine
{
	Vector3::Vector3(int x, int y, int z)
		: x(x), y(y), z(z)
	{
	}

	Vector3::~Vector3()
	{
	}

	Vector3 Vector3::operator+(const Vector3& other) const
	{
		return Vector3(x + other.x, y + other.y, z + other.z);
	}

	Vector3 Vector3::operator+(const Vector2& other) const
	{
		return Vector3(x + other.x, y + other.y, z);
	}

	Vector3 Vector3::operator-(const Vector3& other) const
	{
		return Vector3(x - other.x, y - other.y, z - other.z);
	}

	Vector3 Vector3::operator-(const Vector2& other) const
	{
		return Vector3(x - other.x, y - other.y, z);
	}

	bool Vector3::operator==(const Vector3& other)
	{
		return (x == other.x && y == other.y && z == other.z);
	}

	bool Vector3::operator!=(const Vector3& other)
	{
		return !(*this == other);
	}
}