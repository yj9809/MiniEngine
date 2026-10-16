#pragma once

#include "Math/Vector2.h"

#include "Common/Common.h"

namespace Engine
{
	/**
	 * 3개의 float 성분을 갖는 값 타입입니다.
	 * 반환형 연산자는 원본을 변경하지 않습니다. 일반 산술·내적·길이 계산은
	 * 입력과 오버플로를 검사하지 않으며, 정규화 함수는 별도의 실패 처리를 제공합니다.
	 */
	class ENGINE_API Vector3
	{
	public:
		/**
		 * 모든 성분이 0인 Vector3를 생성합니다.
		 */
		Vector3() = default;
		/**
		 * x, y, z 값을 그대로 저장합니다. 자동 정규화나 유효성 검사는 하지 않습니다.
		 */
		Vector3(float x, float y, float z);
		/**
		 * 객체를 소멸합니다. 별도로 소유하는 동적 리소스는 없습니다.
		 */
		~Vector3();

		/**
		 * 대응하는 성분끼리 더한 새 벡터를 반환합니다. 원본은 유지됩니다.
		 */
		Vector3 operator+(const Vector3& other) const;
		/**
		 * 대응하는 성분끼리 뺀 새 벡터를 반환합니다. 원본은 유지됩니다.
		 */
		Vector3 operator-(const Vector3& other) const;
		/**
		 * 모든 성분에 scalar를 곱한 새 벡터를 반환합니다.
		 */
		Vector3 operator*(float scalar) const;
		/**
		 * 모든 성분을 scalar로 나눈 새 벡터를 반환합니다.
		 * 0 나눗셈과 비유한 입력을 검사하지 않으므로 호출부에서 유효한 나눗값을 제공해야 합니다.
		 */
		Vector3 operator/(float scalar) const;

		/**
		 * 모든 성분을 float의 ==로 정확히 비교합니다. 오차 허용 비교는 하지 않습니다.
		 */
		bool operator==(const Vector3& other) const;
		/**
		 * 성분 중 하나라도 정확히 같지 않으면 true를 반환합니다. operator==의 부정입니다.
		 */
		bool operator!=(const Vector3& other) const;

		/**
		 * 대응 성분의 곱을 모두 더한 내적을 반환합니다.
		 * 두 벡터가 단위 벡터이면 결과는 두 방향 사이 각도의 코사인입니다.
		 */
		float Dot(const Vector3& other) const;
		/**
		 * left와 right의 내적을 반환합니다. left.Dot(right)와 같습니다.
		 */
		static float Dot(const Vector3& left, const Vector3& right);

		/**
		 * 자신과 other의 외적을 반환합니다. 결과를 자동 정규화하지 않습니다.
		 * unitX.Cross(unitY)는 unitZ이며, 순서를 바꾸면 부호가 반대가 됩니다.
		 * 평행한 벡터의 외적은 0벡터입니다.
		 */
		Vector3 Cross(const Vector3& other) const;
		/**
		 * left.Cross(right)와 같은 외적을 반환합니다. 입력 순서가 결과 방향을 결정합니다.
		 */
		static Vector3 Cross(const Vector3& left, const Vector3& right);

		/**
		 * 각 성분의 제곱합(x*x + y*y + z*z)을 반환합니다.
		 * 제곱근 없이 길이를 비교할 때 사용합니다. float 범위 초과는 검사하지 않습니다.
		 */
		float LengthSquared() const;
		/**
		 * LengthSquared()의 제곱근을 반환합니다. float 범위 초과는 검사하지 않습니다.
		 */
		float Length() const;
		/**
		 * 자신의 길이를 1로 정규화합니다. 성분 사이의 비율은 유지합니다.
		 * 길이의 제곱은 double로 계산하며, 기준값 이하면 실패합니다.
		 * 기준값이 음수 또는 비유한 값이거나, 입력·계산 결과가 비유한 값이면 실패합니다.
		 * 실패 시 자신의 원래 값을 유지합니다.
		 * @param minLengthSquared 허용할 길이 제곱의 하한. 유한한 0 이상의 값이어야 합니다.
		 * @return 성공하면 true, 실패하면 false.
		 */
		bool Normalize(float minLengthSquared = 1e-8f);
		/**
		 * 벡터의 길이를 1로 만든 결과를 outResult에 저장합니다.
		 * 길이의 제곱은 double로 계산하며, 기준값 이하면 실패합니다.
		 * 기준값이 음수 또는 비유한 값이거나, 입력·계산 결과가 비유한 값이면 실패합니다.
		 * 실패 시 outResult에 0벡터를 저장합니다. 출력이 별도 객체이면 원본은 유지됩니다.
		 * @param minLengthSquared 허용할 길이 제곱의 하한. 유한한 0 이상의 값이어야 합니다.
		 * @param outResult 정규화 결과를 받을 객체. 자신을 전달하면 자신이 변경됩니다.
		 * @return 성공하면 true, 실패하면 false.
		 */
		bool TryNormalize(Vector3& outResult, float minLengthSquared = 1e-8f) const;

		/**
		 * 모든 성분이 0인 벡터.
		 */
		static const Vector3 zero;
		/**
		 * 모든 성분이 1인 벡터. 단위 길이 벡터는 아닙니다.
		 */
		static const Vector3 one;
		/**
		 * x 성분만 1이고 나머지는 0인 단위 벡터.
		 */
		static const Vector3 unitX;
		/**
		 * y 성분만 1이고 나머지는 0인 단위 벡터.
		 */
		static const Vector3 unitY;
		/**
		 * z 성분만 1이고 나머지는 0인 단위 벡터.
		 */
		static const Vector3 unitZ;
		/**
		 * 월드 위쪽 방향 (0, 0, 1). 이 엔진의 Z-Up 규약을 따릅니다.
		 */
		static const Vector3 up;

	public:
		float x = 0;
		float y = 0;
		float z = 0;
	};
}

