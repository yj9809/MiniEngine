#pragma once

#include "Common/RTTI.h"

namespace Engine
{
	// 2D 부동소수점 벡터.
	// 위치, 방향, 속도 등 화면 공간 연산에 사용된다.
	// 렌더러가 정수 좌표를 요구하는 경우 Actor::GetPositionI()로 변환해 사용한다.
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

		static const Vector2 zero;
		static const Vector2 one;
		static const Vector2 up;
		static const Vector2 down;
		static const Vector2 left;
		static const Vector2 right;

	public:
		float x = 0;
		float y = 0;
	};
}


