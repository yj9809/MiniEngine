#pragma once

#include "Vector3.h"
#include "Vector4.h"

namespace Engine
{
    class Quaternion;

    /**
     * 4x4 float 행렬 값 타입입니다. 반환형 연산과 변환은 원본을 변경하지 않습니다.
     * m00~m33을 행별로 저장하며 이동 성분은 m30, m31, m32입니다.
     * 벡터 연산자 표기는 M * v이지만 실제 성분 계산은 행벡터 v에 M을 오른쪽에서 곱하는 규칙입니다.
     * 별도 설명이 없는 산술·생성 함수는 입력이나 비유한 결과를 검사하지 않습니다.
     */
    class ENGINE_API Matrix4
    {
    public:
        /**
         * 대각 성분이 1이고 나머지는 0인 항등행렬을 생성합니다.
         */
        Matrix4();

        /**
         * 자신과 other의 행렬 곱을 반환합니다. 일반적으로 곱셈 순서를 바꿀 수 없습니다.
         * 이 클래스의 벡터 변환 규칙에서 A * B는 A 변환 후 B 변환을 적용합니다.
         */
        Matrix4 operator*(const Matrix4& other) const;
        /**
         * other를 이 행렬로 변환한 동차좌표를 반환합니다. 원근 나눗셈은 하지 않습니다.
         * 위치는 w=1, 방향은 w=0으로 전달하면 이동 성분의 적용 여부를 구분할 수 있습니다.
         */
        Vector4 operator*(const Vector4& other) const;

        /**
         * position을 w=1로 확장해 변환한 뒤 xyz를 반환합니다. 이동 성분이 적용됩니다.
         * 결과를 w로 나누지 않으므로 투영 후의 원근 나눗셈 용도로 사용하지 않습니다.
         */
        Vector3 TransformPosition(const Vector3& position) const;
        /**
         * vector를 w=0으로 확장해 변환한 뒤 xyz를 반환합니다. 이동 성분은 적용되지 않습니다.
         * 결과를 정규화하지 않으며, 비균일 스케일이 있을 때 법선 변환을 대신하지 않습니다.
         */
        Vector3 TransformVector(const Vector3& vector) const;

        /**
         * 행과 열을 교환한 새 행렬을 반환합니다. 원본은 유지됩니다.
         */
        Matrix4 Transpose() const;
        /**
         * matrix의 행과 열을 교환한 새 행렬을 반환합니다. 입력은 유지됩니다.
         */
        static Matrix4 Transpose(const Matrix4& matrix);
        /**
         * 여인수와 행렬식으로 일반 4x4 역행렬을 계산해 반환합니다.
         * 행렬식이 정확히 0이면 원본의 복사본을 반환하며, 성공 여부는 별도로 반환하지 않습니다.
         * 0에 가까운 행렬식과 비유한 값은 검사하지 않습니다. 수치적으로 불안정한 입력에 주의해야 합니다.
         */
        Matrix4 InverseGeneral() const;
        /**
         * 자신의 전치행렬을 반환합니다. 전체 4x4 행렬이 직교행렬일 때만 역행렬과 같습니다.
         * 입력의 직교성을 검사하지 않습니다. 이동이나 일반 스케일이 포함된 변환의 역행렬로 사용하면 안 됩니다.
         */
        Matrix4 Inverse() const;

        /**
         * 각 축으로 x, y, z만큼 이동하는 행렬을 반환합니다.
         */
        static Matrix4 Translation(float x, float y, float z);
        /**
         * position의 성분을 각 축의 이동량으로 사용하는 행렬을 반환합니다.
         */
        static Matrix4 Translation(const Vector3& position);
        /**
         * 세 축에 같은 배율을 적용하는 행렬을 반환합니다. 1은 유지, 0은 축을 축소합니다.
         */
        static Matrix4 Scale(float scale);
        /**
         * 각 축에 x, y, z 배율을 적용하는 행렬을 반환합니다. 음수 배율은 해당 축을 반전합니다.
         */
        static Matrix4 Scale(float x, float y, float z);
        /**
         * scale의 각 성분을 축별 배율로 사용하는 행렬을 반환합니다.
         */
        static Matrix4 Scale(const Vector3& scale);
        /**
         * X축 회전 행렬을 반환합니다. radians는 라디안 단위입니다.
         * 양의 90도 회전은 +Y 방향을 +Z 방향으로 보냅니다.
         */
        static Matrix4 RotationX(float radians);
        /**
         * Y축 회전 행렬을 반환합니다. radians는 라디안 단위입니다.
         * 양의 90도 회전은 +Z 방향을 +X 방향으로 보냅니다.
         */
        static Matrix4 RotationY(float radians);
        /**
         * Z축 회전 행렬을 반환합니다. radians는 라디안 단위입니다.
         * 양의 90도 회전은 +X 방향을 +Y 방향으로 보냅니다.
         */
        static Matrix4 RotationZ(float radians);
        /**
         * 언리얼식 Euler 방향으로 회전 행렬을 생성합니다.
         * roll, pitch, yaw는 각각 X, Y, Z축의 라디안 각도입니다.
         * RotationX(-roll) * RotationY(-pitch) * RotationZ(yaw)로 합성합니다.
         * 고정축 기준 X → Y → Z 순으로 적용됩니다.
         */
        static Matrix4 Rotation(float roll, float pitch, float yaw);
        /**
         * rotation.x=Roll, y=Pitch, z=Yaw이며 라디안 단위입니다.
         * Rotation(rotation.x, rotation.y, rotation.z)와 같습니다.
         */
        static Matrix4 Rotation(const Vector3& rotation);

        /**
         * 유한한 단위 Quaternion이 나타내는 회전 행렬을 생성합니다.
         * 입력의 정규화와 유효성 검사는 하지 않습니다.
         * 행벡터 v * M 규칙이며, q * v * Conjugate(q)와 같은 회전입니다.
         */
        static Matrix4 Rotation(const Quaternion& rotation);
        /**
         * eye에서 at을 바라보는 왼손 뷰 행렬을 반환합니다. 뷰 공간의 앞쪽은 +Z입니다.
         * @param eye 월드 공간 카메라 위치.
         * @param at 월드 공간에서 바라볼 지점. 방향 벡터가 아닙니다.
         * @param up 위쪽 방향의 기준 벡터. 바라볼 방향과 평행하지 않아야 합니다.
         * @return TryLookAt이 성공하면 계산한 행렬, 실패하면 Identity. 실패 여부가 필요하면 TryLookAt을 사용합니다.
         */
        static Matrix4 LookAt(const Vector3& eye, const Vector3& at, const Vector3& up);
        /**
         * eye에서 at을 바라보는 왼손 뷰 행렬을 outResult에 저장합니다.
         * at-eye와 up.Cross(forward)를 Vector3::TryNormalize의 기본 기준으로 정규화합니다.
         * 방향 또는 옆축 정규화에 실패하거나 최종 행렬에 비유한 성분이 있으면 실패합니다.
         * @param eye 월드 공간 카메라 위치.
         * @param at 월드 공간에서 바라볼 지점.
         * @param up 위쪽 방향의 기준 벡터. 영벡터 또는 바라볼 방향과 평행한 벡터는 사용할 수 없습니다.
         * @param outResult 성공 시 뷰 행렬, 실패 시 Identity가 저장됩니다.
         * @return 성공하면 true, 실패하면 false. 이전 View 유지 여부는 호출부가 결정합니다.
         */
        static bool TryLookAt(const Vector3& eye, const Vector3& at, const Vector3& up, Matrix4& outResult);
        /**
         * 왼손 좌표계의 원근 투영 행렬을 반환합니다. 원근 나눗셈 후 깊이 범위는 0~1입니다.
         * @param fovY 수직 시야각. 회전 함수와 달리 도(degree) 단위이며 0 < fovY < 180이어야 합니다.
         * @param aspect 뷰포트의 가로/세로 비율. 0보다 커야 합니다.
         * @param nearZ 가까운 절단면 거리. 0보다 커야 합니다.
         * @param farZ 먼 절단면 거리. nearZ보다 커야 합니다.
         * 입력은 유한해야 합니다. 함수 내부에서 범위 검사나 실패 대체 처리를 하지 않습니다.
         */
        static Matrix4 PerspectiveFOV(float fovY, float aspect, float nearZ, float farZ);

        /**
         * 변환을 적용하지 않는 항등행렬.
         */
        static const Matrix4 identity;
        /**
         * 모든 성분이 0인 행렬. 항등행렬과 다릅니다.
         */
        static const Matrix4 zero;

    private:
        /**
         * a,b,c / d,e,f / g,h,i를 세 행으로 하는 3x3 행렬의 행렬식을 반환합니다.
         * InverseGeneral의 여인수 계산에 사용합니다.
         */
        static float det3x3(float a, float b, float c, float d, float e, float f, float g, float h, float i);

        /**
         * 모든 성분을 0으로 초기화한 행렬을 반환합니다. zero 상수 생성에 사용합니다.
         */
        static inline Matrix4 ZeroMatrix()
        {
            Matrix4 result;
            memset(result.elements, 0, sizeof(float) * 16);
            return result;
        }

    private:
        // 이름으로 접근하는 성분과 elements 배열은 같은 저장 공간을 공유합니다.
        union
        {
            struct
            {
                float m00, m01, m02, m03;
                float m10, m11, m12, m13;
                float m20, m21, m22, m23;
                float m30, m31, m32, m33;
            };

            float elements[16] = {};
        };
    };
}
