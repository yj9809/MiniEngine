#pragma once

#include "Component/Component.h"
#include "Math/Vector3.h"
#include "Common/RTTI.h"

namespace Engine
{
	class Collider : public Component
	{
		RTTI_DECLARATIONS(Collider, Component)

	public:
		Collider() = default;
		Collider(Vector3 center, bool isTrigger)
			: center(center), isTrigger(isTrigger)
		{
		}

		inline Vector3 GetWorldMin() const { return worldMin; }
		inline Vector3 GetWorldMax() const { return worldMax; }

		virtual bool Overlap(const Collider& other) const = 0;

	protected:
		Vector3 center;

		Vector3 worldMin;

		Vector3 worldMax;

		bool isTrigger = false;
	};
}
