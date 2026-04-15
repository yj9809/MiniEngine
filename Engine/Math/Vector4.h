#pragma once

#include "Common/Common.h"
#include "Vector3.h"


namespace Engine
{
	class ENGINE_API Vector4
	{
	public:
		Vector4() = default;
		Vector4(float x, float y, float z, float w);
		Vector4(const Vector3& v, float w);
		~Vector4() = default;

		Vector4 operator+(const Vector4& other) const;
		Vector4 operator-(const Vector4& other) const;
		Vector4 operator*(float scalar) const;
		Vector4 operator/(float scalar) const;

		bool operator==(const Vector4& other) const;
		bool operator!=(const Vector4& other) const;

		// 내적.
		float Dot(const Vector4& other) const;

		// 제곱 길이.
		float LengthSquared() const;
		// Vector4 길이.
		float Length() const;
		// Vector4 정규화.
		Vector4 Normalize() const;
		// Vector3 변환.
		Vector3 ToVector3() const;

		// Vector4 상수.
		static const Vector4 zero;
		static const Vector4 one;
		static const Vector4 unitX;
		static const Vector4 unitY;
		static const Vector4 unitZ;

	public:
		float x = 0;
		float y = 0;
		float z = 0;
		float w = 0;
	};
}

