#include "Matrix4.h"

#include <cmath>

namespace Engine
{
	Matrix4::Matrix4()
	{
		memset(elements, 0, sizeof(float) * 16);

		m00 = m11 = m22 = m33 = 1.0f;
	}

	Matrix4 Matrix4::operator*(const Matrix4& other) const
	{
		Matrix4 result;

		// 첫 번째 행.
		result.m00 = m00 * other.m00 + m01 * other.m10 + m02 * other.m20 + m03 * other.m30;
		result.m01 = m00 * other.m01 + m01 * other.m11 + m02 * other.m21 + m03 * other.m31;
		result.m02 = m00 * other.m02 + m01 * other.m12 + m02 * other.m22 + m03 * other.m32;
		result.m03 = m00 * other.m03 + m01 * other.m13 + m02 * other.m23 + m03 * other.m33;

		// 두 번째 행.
		result.m10 = m10 * other.m00 + m11 * other.m10 + m12 * other.m20 + m13 * other.m30;
		result.m11 = m10 * other.m01 + m11 * other.m11 + m12 * other.m21 + m13 * other.m31;
		result.m12 = m10 * other.m02 + m11 * other.m12 + m12 * other.m22 + m13 * other.m32;
		result.m13 = m10 * other.m03 + m11 * other.m13 + m12 * other.m23 + m13 * other.m33;

		// 세 번째 행.
		result.m20 = m20 * other.m00 + m21 * other.m10 + m22 * other.m20 + m23 * other.m30;
		result.m21 = m20 * other.m01 + m21 * other.m11 + m22 * other.m21 + m23 * other.m31;
		result.m22 = m20 * other.m02 + m21 * other.m12 + m22 * other.m22 + m23 * other.m32;
		result.m23 = m20 * other.m03 + m21 * other.m13 + m22 * other.m23 + m23 * other.m33;

		// 네 번째 행.
		result.m30 = m30 * other.m00 + m31 * other.m10 + m32 * other.m20 + m33 * other.m30;
		result.m31 = m30 * other.m01 + m31 * other.m11 + m32 * other.m21 + m33 * other.m31;
		result.m32 = m30 * other.m02 + m31 * other.m12 + m32 * other.m22 + m33 * other.m32;
		result.m33 = m30 * other.m03 + m31 * other.m13 + m32 * other.m23 + m33 * other.m33;

		return result;
	}

	Vector4 Matrix4::operator*(const Vector4& other) const
	{
		Vector4 result;

		result.x = m00 * other.x + m01 * other.y + m02 * other.z + m03 * other.w;
		result.y = m10 * other.x + m11 * other.y + m12 * other.z + m13 * other.w;
		result.z = m20 * other.x + m21 * other.y + m22 * other.z + m23 * other.w;
		result.w = m30 * other.x + m31 * other.y + m32 * other.z + m33 * other.w;

		return result;
	}

	Vector3 Matrix4::TransformPosition(const Vector3& position) const
	{
		Vector4 v(position, 1.0f);
		Vector4 result = *this * v;
		return result.ToVector3();
	}

	Vector3 Matrix4::TransformVector(const Vector3& vector) const
	{
		Vector4 v(vector, 0.0f);
		Vector4 result = *this * v;
		return result.ToVector3();
	}

	Matrix4 Matrix4::Transpose() const
	{
		return Transpose(*this);
	}

	Matrix4 Matrix4::Transpose(const Matrix4& matrix)
	{
		Matrix4 result = matrix;

		std::swap<float>(result.m01, result.m10);
		std::swap<float>(result.m02, result.m20);
		std::swap<float>(result.m03, result.m30);

		std::swap<float>(result.m12, result.m21);
		std::swap<float>(result.m13, result.m31);

		std::swap<float>(result.m23, result.m32);

		return result;
	}

	Matrix4 Matrix4::InverseGeneral() const
	{
		Matrix4 result;

		// 여인수 구하기.
		float c00 = +det3x3(m11, m12, m13, m21, m22, m23, m31, m32, m33);
		float c01 = -det3x3(m10, m12, m13, m20, m22, m23, m30, m32, m33);
		float c02 = +det3x3(m10, m11, m13, m20, m21, m23, m30, m31, m33);
		float c03 = -det3x3(m10, m11, m12, m20, m21, m22, m30, m31, m32);
			  
		float c10 = -det3x3(m01, m02, m03, m21, m22, m23, m31, m32, m33);
		float c11 = +det3x3(m00, m02, m03, m20, m22, m23, m30, m32, m33);
		float c12 = -det3x3(m00, m01, m03, m20, m21, m23, m30, m31, m33);
		float c13 = +det3x3(m00, m01, m02, m20, m21, m22, m30, m31, m32);
			  
		float c20 = +det3x3(m01, m02, m03, m11, m12, m13, m31, m32, m33);
		float c21 = -det3x3(m00, m02, m03, m10, m12, m13, m30, m32, m33);
		float c22 = +det3x3(m00, m01, m03, m10, m11, m13, m30, m31, m33);
		float c23 = -det3x3(m00, m01, m02, m10, m11, m12, m30, m31, m32);
			  
		float c30 = +det3x3(m01, m02, m03, m11, m12, m13, m21, m22, m23);
		float c31 = -det3x3(m00, m02, m03, m10, m12, m13, m20, m22, m23);
		float c32 = +det3x3(m00, m01, m03, m10, m11, m13, m20, m21, m23);
		float c33 = -det3x3(m00, m01, m02, m10, m11, m12, m20, m21, m22);

		// 행렬식.
		float det = m00 * c00 + m01 * c01 + m02 * c02 + m03 * c03;

		// 역행렬 존재 여부 체크.
		if (det == 0)
		{
			return *this;
		}

		// 수반행렬 (여인수 행렬을 전치).
		result.m00 = c00;
		result.m01 = c10;
		result.m02 = c20;
		result.m03 = c30;

		result.m10 = c01;
		result.m11 = c11;
		result.m12 = c21;
		result.m13 = c31;

		result.m20 = c02;
		result.m21 = c12;
		result.m22 = c22;
		result.m23 = c32;

		result.m30 = c03;
		result.m31 = c13;
		result.m32 = c23;
		result.m33 = c33;

		// 1/det 곱하기.
		float invDet = 1.0f / det;
		for (int i = 0; i < 16; i++)
		{
			result.elements[i] *= invDet;
		}

		return result;
	}

	Matrix4 Matrix4::Inverse() const
	{
		return Transpose(*this);
	}

	Matrix4 Matrix4::Translation(float x, float y, float z)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::Translation(const Vector3& position)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::Scale(float scale)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::Scale(float x, float y, float z)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::Scale(const Vector3& scale)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::RotationX(float radians)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::RotationY(float radians)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::RotationZ(float radians)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::Rotation(float x, float y, float z)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::Rotation(const Vector3& rotation)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::LookAt(const Vector3& eye, const Vector3& at, const Vector3& up)
	{
		return Matrix4();
	}

	Matrix4 Matrix4::PerspectiveFOV(float fovY, float aspect, float nearZ, float farZ)
	{
		return Matrix4();
	}

	float Matrix4::det3x3(float a, float b, float c, float d, float e, float f, float g, float h, float i)
	{
		// a b c
		// d e f
		// g h i

		return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
	}
}