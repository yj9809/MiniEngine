#pragma once

#include "Vector3.h"
#include "Vector4.h"

namespace Engine
{
	class ENGINE_API Matrix4
	{
	public:
		Matrix4();

		Matrix4 operator*(const Matrix4& other) const;
		Vector4 operator*(const Vector4& other) const;

		// 포지션 변화.
		Vector3 TransformPosition(const Vector3& position) const;
		Vector3 TransformVector(const Vector3& vector) const;

		// 전치.
		Matrix4 Transpose() const;
		static Matrix4 Transpose(const Matrix4& matrix);
		// 범용 역행렬 (사용 목적보다는 실제 구현해보는 목적).
		Matrix4 InverseGeneral() const;
		// 직교행렬 역행렬 (직교행렬은 전치만 해주면 된다).
		Matrix4 Inverse() const;

		// 이동 행렬.
		static Matrix4 Translation(float x, float y, float z);
		static Matrix4 Translation(const Vector3& position);
		// 크기 행렬.
		static Matrix4 Scale(float scale);
		static Matrix4 Scale(float x, float y, float z);
		static Matrix4 Scale(const Vector3& scale);
		// 회전 행렬.
		// ZXY 기준으로 작성.
		static Matrix4 RotationX(float radians);
		static Matrix4 RotationY(float radians);
		static Matrix4 RotationZ(float radians);
		static Matrix4 Rotation(float x, float y, float z);
		static Matrix4 Rotation(const Vector3& rotation);
		// 카메라 변환 행렬.
		// eye: 카메라 위치, at: 바라보는 지점, up: 월드 업 벡터.
		static Matrix4 LookAt(const Vector3& eye, const Vector3& at, const Vector3& up);
		// 투영 행렬.
		// 왼손 좌표계.
		static Matrix4 PerspectiveFOV(float fovY, float aspect, float nearZ, float farZ);

		static const Matrix4 identity;
		static const Matrix4 zero;

	private:
		// 소행렬식.
		static float det3x3(float a, float b, float c, float d, float e, float f, float g, float h, float i);

	private:
		// 공용체로 지정.
		// elements와 struct는 같은 메모리 공간을 공유.
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

