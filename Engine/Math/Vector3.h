#pragma once

#include "Math/Vector2.h"

#include "Common/Common.h"

namespace Engine
{
	class ENGINE_API Vector3
	{
	public:
		Vector3() = default;
		Vector3(float x, float y, float z);
		~Vector3();

		Vector3 operator+(const Vector3& other) const;
		Vector3 operator-(const Vector3& other) const;
		Vector3 operator*(float scalar) const;
		Vector3 operator/(float scalar) const;

		bool operator==(const Vector3& other) const;
		bool operator!=(const Vector3& other) const;

		// 내적.
		float Dot(const Vector3& other) const;
		static float Dot(const Vector3& left, const Vector3& right);

		// 외적.
		Vector3 Cross(const Vector3& other) const;
		static Vector3 Cross(const Vector3& left, const Vector3& right);

		// 제곱 길이.
		float LengthSquared() const;
		// Vector3 길이.
		float Length() const;
		// 정규화.
		Vector3 Normalize() const;

		// Vector3 상수.
		static const Vector3 zero;
		static const Vector3 one;
		static const Vector3 unitX;
		static const Vector3 unitY;
		static const Vector3 unitZ;
		static const Vector3 up;

	public:
		float x = 0;
		float y = 0;
		float z = 0;
	};
}

