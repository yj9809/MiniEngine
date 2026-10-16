#pragma once

#include "Common/Common.h"
#include "Vector3.h"


namespace Engine
{
	/**
	 * 4개의 float 성분을 갖는 값 타입입니다.
	 * 반환형 연산자는 원본을 변경하지 않습니다. 일반 산술·내적·길이 계산은
	 * 입력과 오버플로를 검사하지 않으며, 정규화 함수는 별도의 실패 처리를 제공합니다.
	 */
	class ENGINE_API Vector4
	{
	public:
		/**
		 * 모든 성분이 0인 Vector4를 생성합니다.
		 */
		Vector4() = default;
		/**
		 * x, y, z, w 값을 그대로 저장합니다. 자동 정규화나 유효성 검사는 하지 않습니다.
		 */
		Vector4(float x, float y, float z, float w);
		/**
		 * v의 x, y, z를 복사하고 지정한 w를 저장합니다.
		 * 동차좌표에서 위치는 w=1, 방향은 w=0으로 표현할 수 있습니다.
		 */
		Vector4(const Vector3& v, float w);
		/**
		 * 객체를 소멸합니다. 별도로 소유하는 동적 리소스는 없습니다.
		 */
		~Vector4() = default;

		/**
		 * 대응하는 성분끼리 더한 새 벡터를 반환합니다. 원본은 유지됩니다.
		 */
		Vector4 operator+(const Vector4& other) const;
		/**
		 * 대응하는 성분끼리 뺀 새 벡터를 반환합니다. 원본은 유지됩니다.
		 */
		Vector4 operator-(const Vector4& other) const;
		/**
		 * 모든 성분에 scalar를 곱한 새 벡터를 반환합니다.
		 */
		Vector4 operator*(float scalar) const;
		/**
		 * 모든 성분을 scalar로 나눈 새 벡터를 반환합니다.
		 * 0 나눗셈과 비유한 입력을 검사하지 않으므로 호출부에서 유효한 나눗값을 제공해야 합니다.
		 */
		Vector4 operator/(float scalar) const;

		/**
		 * 모든 성분을 float의 ==로 정확히 비교합니다. 오차 허용 비교는 하지 않습니다.
		 */
		bool operator==(const Vector4& other) const;
		/**
		 * 성분 중 하나라도 정확히 같지 않으면 true를 반환합니다. operator==의 부정입니다.
		 */
		bool operator!=(const Vector4& other) const;

		/**
		 * 대응 성분의 곱을 모두 더한 내적을 반환합니다.
		 * w도 계산에 포함됩니다.
		 */
		float Dot(const Vector4& other) const;
		/**
		 * left와 right의 내적을 반환합니다. left.Dot(right)와 같습니다.
		 */
		static float Dot(const Vector4& left, const Vector4& right);

		/**
		 * 각 성분의 제곱합(x*x + y*y + z*z + w*w)을 반환합니다.
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
		bool TryNormalize(Vector4& outResult, float minLengthSquared = 1e-8f) const;
		/**
		 * x, y, z를 복사한 Vector3를 반환합니다. w는 버리며 w로 나누지 않습니다.
		 */
		Vector3 ToVector3() const;

		/**
		 * 모든 성분이 0인 벡터.
		 */
		static const Vector4 zero;
		/**
		 * 모든 성분이 1인 벡터. 단위 길이 벡터는 아닙니다.
		 */
		static const Vector4 one;
		/**
		 * x 성분만 1이고 나머지는 0인 단위 벡터.
		 */
		static const Vector4 unitX;
		/**
		 * y 성분만 1이고 나머지는 0인 단위 벡터.
		 */
		static const Vector4 unitY;
		/**
		 * z 성분만 1이고 나머지는 0인 단위 벡터.
		 */
		static const Vector4 unitZ;

	public:
		float x = 0;
		float y = 0;
		float z = 0;
		float w = 0;
	};
}

