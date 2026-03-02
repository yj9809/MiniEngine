#include "Vector2.h"
namespace Engine
{
	Vector2::Vector2(float x, float y)
		: x(x), y(y)
	{

	}

	Vector2 Vector2::operator+(const Vector2& other) const
	{
		return Vector2(x + other.x, y + other.y);
	}

	Vector2 Vector2::operator-(const Vector2& other) const
	{
		return Vector2(x - other.x, y - other.y);
	}

	bool Vector2::operator==(const Vector2& other) const
	{
		return (x == other.x && y == other.y);
	}

	bool Vector2::operator!=(const Vector2& other) const
	{
		return !(*this == other);
	}
}