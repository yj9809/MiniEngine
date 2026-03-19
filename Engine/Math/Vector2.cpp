#include "Vector2.h"
namespace Engine
{
	const Vector2 Vector2::zero = Vector2(0, 0);
	const Vector2 Vector2::one = Vector2(1, 1);
	const Vector2 Vector2::up = Vector2(0, -1);
	const Vector2 Vector2::down = Vector2(0, 1);
	const Vector2 Vector2::left = Vector2(-1, 0);
	const Vector2 Vector2::right = Vector2(1, 0);

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