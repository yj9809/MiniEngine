#pragma once

#include "Math/Vector2.h"

#include "Common/Common.h"

namespace Engine
{
	class ENGINE_API Vector3
	{
	public:
		Vector3() = default;
		Vector3(int x, int y, int z);
		~Vector3();

		Vector3 operator+(const Vector3& other) const;
		Vector3 operator+(const Vector2& other) const;

		Vector3 operator-(const Vector3& other) const;
		Vector3 operator-(const Vector2& other) const;

		bool operator==(const Vector3& other);
		bool operator!=(const Vector3& other);

	public:
		float x = 0;
		float y = 0;
		float z = 0;
	};
}

