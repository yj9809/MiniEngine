#pragma once

#include "Common/Common.h"

namespace Engine
{
    class Vector3;
    /**
     * (x, y, z, w) 순서의 쿼터니언 값 타입입니다. xyz는 벡터부, w는 스칼라부입니다.
     * 회전에는 단위 쿼터니언을 사용합니다. 생성자는 자동 정규화하지 않습니다.
     */
    class ENGINE_API Quaternion
    {
    public:
        /**
         * 회전하지 않는 Identity (0, 0, 0, 1)로 초기화합니다.
         */
        Quaternion() = default;
        /**
         * 네 성분을 그대로 저장합니다. 정규화와 입력 검증은 별도로 수행해야 합니다.
         */
        Quaternion(float x, float y, float z, float w);

        Quaternion operator*(const Quaternion& other) const;

        /**
         * 자신의 길이를 1로 정규화합니다. 성분 사이의 비율은 유지합니다.
         * 길이의 제곱은 double로 계산하며, 기준값 이하면 실패합니다.
         * 기준값이 음수 또는 비유한 값이거나, 입력·계산 결과가 비유한 값이면 실패합니다.
         * 실패 시 자신을 Identity로 변경합니다.
         * @param minLengthSquared 허용할 길이 제곱의 하한. 유한한 0 이상의 값이어야 합니다.
         * @return 성공하면 true, 실패하면 false.
         */
        bool Normalize(float minLengthSquared = 1e-8f);

        /**
         * 쿼터니언의 길이를 1로 만든 결과를 outResult에 저장합니다.
         * 길이의 제곱은 double로 계산하며, 기준값 이하면 실패합니다.
         * 기준값이 음수 또는 비유한 값이거나, 입력·계산 결과가 비유한 값이면 실패합니다.
         * 실패 시 outResult에 Identity를 저장합니다. 출력이 별도 객체이면 원본은 유지됩니다.
         * @param minLengthSquared 허용할 길이 제곱의 하한. 유한한 0 이상의 값이어야 합니다.
         * @param outResult 정규화 결과를 받을 객체. 자신을 전달하면 자신이 변경됩니다.
         * @return 성공하면 true, 실패하면 false.
         */
        bool TryNormalize(Quaternion& outResult, float minLengthSquared = 1e-8f) const;

        /**
         * 자신의 x, y, z 부호를 반전하고 w는 유지합니다.
         * 단위 쿼터니언에서는 켤레가 회전을 되돌리는 역원과 같습니다.
         * 정규화나 비유한 값 검사는 하지 않습니다.
         */
        void ConjugateInPlace();

        /**
         * (-x, -y, -z, w)를 새 쿼터니언으로 반환하며 원본은 유지합니다.
         * 단위 쿼터니언에서는 역원과 같습니다. 일반 역원은 켤레를 길이 제곱으로 나눠야 합니다.
         * 정규화나 비유한 값 검사는 하지 않습니다.
         */
        Quaternion Conjugate() const;

        /**
         * 자신을 역원(켤레 / 길이 제곱)으로 변경합니다. 단위 길이가 아닌 입력도 계산합니다.
         * TryInvert와 같은 입력·결과 검사를 수행하고, 실패하면 자신을 Identity로 변경합니다.
         * @param minLengthSquared 허용할 길이 제곱의 하한. 유한한 0 이상의 값이어야 합니다.
         * @return 성공하면 true, 실패하면 false.
         */
        bool Invert(float minLengthSquared = 1e-8f);

        /**
         * 켤레를 길이 제곱으로 나눈 역원을 outResult에 저장합니다. 제곱근은 사용하지 않습니다.
         * 길이 제곱과 나눗셈은 double로 계산합니다. 길이 제곱이 기준값 이하면 실패합니다.
         * 기준값이 음수 또는 비유한 값이거나, 입력·계산 결과가 비유한 값이면 실패합니다.
         * 결과 성분의 절댓값이 float 최대값을 초과해도 실패합니다.
         * 실패 시 outResult에 Identity를 저장합니다. 출력이 별도 객체이면 원본은 유지됩니다.
         * @param outResult 역원을 받을 객체. 자신을 전달하면 자신이 변경됩니다.
         * @param minLengthSquared 허용할 길이 제곱의 하한. 유한한 0 이상의 값이어야 합니다.
         * @return 성공하면 true, 실패하면 false.
         */
        bool TryInvert(Quaternion& outResult, float minLengthSquared = 1e-8f) const;

        static bool TryFromAxisAngle(const Vector3& axis, float radians, Quaternion& outResult);

        /**
         * 회전하지 않는 단위 쿼터니언 (0, 0, 0, 1).
         */
        static const Quaternion identity;

    public:
        float x = 0;
        float y = 0;
        float z = 0;
        float w = 1;
    };
}