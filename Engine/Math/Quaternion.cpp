#include "Quaternion.h"

#include <cmath>
#include <limits>
#include <algorithm>

#include "Vector3.h"

namespace Engine
{
    Quaternion::Quaternion(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w)
    {

    }

    Quaternion Quaternion::operator*(const Quaternion& other) const
    {
        return Quaternion(
          w * other.x + x * other.w + y * other.z - z * other.y,
          w * other.y + y * other.w + z * other.x - x * other.z,
          w * other.z + z * other.w + x * other.y - y * other.x,
          w * other.w - x * other.x - y * other.y - z * other.z
        );
    }

    bool Quaternion::Normalize(float minLengthSquared)
    {
        Quaternion normalized;
        if (TryNormalize(normalized, minLengthSquared))
        {
            x = normalized.x;
            y = normalized.y;
            z = normalized.z;
            w = normalized.w;
            return true;
        }
        *this = Quaternion::identity;
        return false;
    }

    bool Quaternion::TryNormalize(Quaternion& outResult, float minLengthSquared) const
    {
        if (!std::isfinite(minLengthSquared) || minLengthSquared < 0.0f)
        {
            outResult = Quaternion::identity;
            return false;
        }

        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(w))
        {
            outResult = Quaternion::identity;
            return false;
        }

        const double dx = static_cast<double>(x);
        const double dy = static_cast<double>(y);
        const double dz = static_cast<double>(z);
        const double dw = static_cast<double>(w);

        const double lenSquared = (dx * dx) + (dy * dy) + (dz * dz) + (dw * dw);
        if (lenSquared <= minLengthSquared)
        {
            outResult = Quaternion::identity;
            return false;
        }

        const double len = sqrt(lenSquared);

        const double rx = dx / len;
        const double ry = dy / len;
        const double rz = dz / len;
        const double rw = dw / len;

        if (!std::isfinite(rx) || !std::isfinite(ry) || !std::isfinite(rz) || !std::isfinite(rw))
        {
            outResult = Quaternion::identity;
            return false;
        }

        outResult = Quaternion(static_cast<float>(rx), static_cast<float>(ry), static_cast<float>(rz), static_cast<float>(rw));
        return true;
    }

    void Quaternion::ConjugateInPlace()
    {
        *this = Conjugate();
    }

    Quaternion Quaternion::Conjugate() const
    {
        return Quaternion(-x, -y, -z, w);
    }

    bool Quaternion::Invert(float minLengthSquared)
    {
        Quaternion inverted;
        const bool success = TryInvert(inverted, minLengthSquared);
        *this = inverted;
        return success;
    }

    bool Quaternion::TryInvert(Quaternion& outResult, float minLengthSquared) const
    {
        if (!std::isfinite(minLengthSquared) || minLengthSquared < 0.0f)
        {
            outResult = Quaternion::identity;
            return false;
        }

        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(w))
        {
            outResult = Quaternion::identity;
            return false;
        }

        const double dx = static_cast<double>(x);
        const double dy = static_cast<double>(y);
        const double dz = static_cast<double>(z);
        const double dw = static_cast<double>(w);

        const double lenSquared = (dx * dx) + (dy * dy) + (dz * dz) + (dw * dw);
        if (lenSquared <= minLengthSquared)
        {
            outResult = Quaternion::identity;
            return false;
        }

        const double rx = -dx / lenSquared;
        const double ry = -dy / lenSquared;
        const double rz = -dz / lenSquared;
        const double rw = dw / lenSquared;

        if (!std::isfinite(rx) || !std::isfinite(ry) || !std::isfinite(rz) || !std::isfinite(rw))
        {
            outResult = Quaternion::identity;
            return false;
        }

        const double maxFloat = (std::numeric_limits<float>::max)();

        if(std::abs(rx) > maxFloat || std::abs(ry) > maxFloat || std::abs(rz) > maxFloat || std::abs(rw) > maxFloat)
        {
            outResult = Quaternion::identity;
            return false;
        }

        outResult = Quaternion(static_cast<float>(rx), static_cast<float>(ry), static_cast<float>(rz), static_cast<float>(rw));
        return true;
    }

    bool Quaternion::TryFromAxisAngle(const Vector3& axis, float radians, Quaternion& outResult)
    {
        if (!std::isfinite(radians))
        {
            outResult = Quaternion::identity;
            return false;
        }

        Vector3 normalizedAxis;

        if (!axis.TryNormalize(normalizedAxis))
        {
            outResult = Quaternion::identity;
            return false;
        }

        const float halfAngle = radians * 0.5f;
        const float s = std::sin(halfAngle);
        const float c = std::cos(halfAngle);

        outResult = Quaternion(normalizedAxis.x * s, normalizedAxis.y * s, normalizedAxis.z * s, c);
        return true;
    }

    bool Quaternion::TryFromEuler(float roll, float pitch, float yaw, Quaternion& outResult)
    {
        Quaternion qRoll, qPitch, qYaw;

        if (!TryFromAxisAngle(Vector3::unitX, -roll, qRoll) ||
            !TryFromAxisAngle(Vector3::unitY, -pitch, qPitch) ||
            !TryFromAxisAngle(Vector3::unitZ, yaw, qYaw))
        {
            outResult = Quaternion::identity;
            return false;
        }

        const Quaternion combined = qYaw * qPitch * qRoll;
        return combined.TryNormalize(outResult);
    }

    bool Quaternion::TryFromEuler(const Vector3& euler, Quaternion& outResult)
    {
        return TryFromEuler(euler.x, euler.y, euler.z, outResult);
    }

    bool Quaternion::TryRotateVector(const Vector3& vector, Vector3& outResult) const
    {
        if (!std::isfinite(vector.x) || !std::isfinite(vector.y) || !std::isfinite(vector.z))
        {
            outResult = Vector3::zero;
            return false;
        }

        // Quaternion(this)의 정규화된 복사본을 만든다.
        Quaternion rotation;
        if (!TryNormalize(rotation))
        {
            outResult = Vector3::zero;
            return false;
        }

        // 입력 벡터를 스칼라부가 0인 Quaternion으로 변환한다.
        const Quaternion p(vector.x, vector.y, vector.z, 0.0f);

        // rotation은 단위 Quaternion이므로 역원 대신 켤레를 사용한다.
        const Quaternion rotated = rotation * p * rotation.Conjugate();

        if (!std::isfinite(rotated.x) || !std::isfinite(rotated.y) || !std::isfinite(rotated.z))
        {
            outResult = Vector3::zero;
            return false;
        }

        outResult = Vector3(rotated.x, rotated.y, rotated.z);
        return true;
    }

    bool Quaternion::TryNlerp(const Quaternion& from, const Quaternion& to, float t, Quaternion& outResult)
    {
        // 보간 파라미터가 유효한지 확인.
        if (!std::isfinite(t) || t < 0.0f || t > 1.0f)
        {
            outResult = Quaternion::identity;
            return false;
        }

        // 두 Quaternion을 정규화.
        Quaternion normalizedFrom, normalizedTo;
        if (!from.TryNormalize(normalizedFrom) || !to.TryNormalize(normalizedTo))
        {
            outResult = Quaternion::identity;
            return false;
        }

        // 정규화된 두 Quaternion의 네 성분 내적.
        const float dot = normalizedFrom.x * normalizedTo.x +
                          normalizedFrom.y * normalizedTo.y +
                          normalizedFrom.z * normalizedTo.z +
                          normalizedFrom.w * normalizedTo.w;

        // 같은 목표 자세를 나타내는 두 부호 중 최단 경로를 선택한다.
        if (dot < 0.0f)
        {
            normalizedTo = Quaternion(-normalizedTo.x, -normalizedTo.y, -normalizedTo.z, -normalizedTo.w);
        }

        // 각 성분을 (1 - t) : t 비율로 섞는다.
        const float fromWeight = 1.0f - t;

        const Quaternion blended(
            fromWeight * normalizedFrom.x + t * normalizedTo.x,
            fromWeight * normalizedFrom.y + t * normalizedTo.y,
            fromWeight * normalizedFrom.z + t * normalizedTo.z,
            fromWeight * normalizedFrom.w + t * normalizedTo.w
        );

        // 선형 보간 결과는 단위 길이가 아닐 수 있으므로 정규화한다.
        // 실패 시 Identity를 반환한다.
        return blended.TryNormalize(outResult);
    }

    bool Quaternion::TrySlerp(const Quaternion& from, const Quaternion& to, float t, Quaternion& outResult)
    {
        // 보간 파라미터가 유효한지 확인.
        if (!std::isfinite(t) || t < 0.0f || t > 1.0f)
        {
            outResult = Quaternion::identity;
            return false;
        }

        // 두 Quaternion을 정규화.
        Quaternion normalizedFrom, normalizedTo;
        if (!from.TryNormalize(normalizedFrom) || !to.TryNormalize(normalizedTo))
        {
            outResult = Quaternion::identity;
            return false;
        }

        // 정규화된 두 Quaternion의 네 성분 내적.
        float dot = normalizedFrom.x * normalizedTo.x +
                          normalizedFrom.y * normalizedTo.y +
                          normalizedFrom.z * normalizedTo.z +
                          normalizedFrom.w * normalizedTo.w;

        // 같은 목표 자세를 나타내는 두 부호 중 최단 경로를 선택한다.
        if (dot < 0.0f)
        {
            normalizedTo = Quaternion(-normalizedTo.x, -normalizedTo.y, -normalizedTo.z, -normalizedTo.w);

            dot = -dot;
        }

        // 정규화 오차로 dot이 1을 조금 넘더라도 acos가 실패하지 않도록 한다.
        dot = std::clamp(dot, 0.0f, 1.0f);

        // 두 자세가 매우 가까우면 선형 보간으로 대체한다.
        if (dot > 0.9995f)
        {
            return TryNlerp(normalizedFrom, normalizedTo, t, outResult);
        }

        // Quaternion 구면에서 두 점 사이의 각도.
        const float theta = std::acos(dot);
        const float sinTheta = std::sin(theta);

        // 구면 위에서 일정한 각도 비율로 이동하도록 가중치를 계산한다.
        const float fromWeight = std::sin((1.0f - t) * theta) / sinTheta;
        const float toWeight = std::sin(t * theta) / sinTheta;

        const Quaternion blended(
            fromWeight * normalizedFrom.x + toWeight * normalizedTo.x,
            fromWeight * normalizedFrom.y + toWeight * normalizedTo.y,
            fromWeight * normalizedFrom.z + toWeight * normalizedTo.z,
            fromWeight * normalizedFrom.w + toWeight * normalizedTo.w
        );

        // 이론적으로 단위 길이지만, 실제 계산의 반올림 오차를 보정한다.
        return blended.TryNormalize(outResult);
    }

    const Quaternion Quaternion::identity = Quaternion(0, 0, 0, 1);
}