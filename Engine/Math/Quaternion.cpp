#include "Quaternion.h"

#include <cmath>
#include <limits>

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

    const Quaternion Quaternion::identity = Quaternion(0, 0, 0, 1);
}