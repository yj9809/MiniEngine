#include "Vector2.h"

#include <cmath>

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

	bool Vector2::Normalize(float minLengthSquared)
	{
		Vector2 normalized;
		if (TryNormalize(normalized, minLengthSquared))
		{
			x = normalized.x;
			y = normalized.y;
			return true;
		}

		return false;
	}

	bool Vector2::TryNormalize(Vector2& outResult, float minLengthSquared) const
	{
		if (!std::isfinite(minLengthSquared) || minLengthSquared < 0.0f)
		{
			outResult = Vector2::zero;
			return false;
		}

		if (!std::isfinite(x) || !std::isfinite(y))
		{
			outResult = Vector2::zero;
			return false;
		}

		const double dx = static_cast<double>(x);
		const double dy = static_cast<double>(y);

		const double lenSquared = (dx * dx) + (dy * dy);
		if (lenSquared <= minLengthSquared)
		{
			outResult = Vector2::zero;
			return false;
		}

		const double len = sqrt(lenSquared);

		const double rx = dx / len;
		const double ry = dy / len;
		if (!std::isfinite(rx) || !std::isfinite(ry))
		{
			outResult = Vector2::zero;
			return false;
		}

		outResult = Vector2(static_cast<float>(rx), static_cast<float>(ry));
		return true;
	}
}