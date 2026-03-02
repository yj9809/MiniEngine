#pragma once

#include "Common/RTTI.h"

namespace Engine
{
	class ENGINE_API Vector2
	{
	public:
		Vector2() = default;
		Vector2(float x, float y);
		~Vector2() = default;

		Vector2 operator+(const Vector2& other) const;
		Vector2 operator-(const Vector2& other) const;

		bool operator==(const Vector2& other) const;
		bool operator!=(const Vector2& other) const;

	public:
		float x = 0;
		float y = 0;
	};
}

