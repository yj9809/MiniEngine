#pragma once

#include "Component/Physics/Collider.h"

#include "Common/RTTI.h"
#include "Common/Common.h"
#include "Math/Vector3.h"

#include <vector>

namespace Engine
{
	class ENGINE_API BoxCollider : public Collider
	{
		RTTI_DECLARATIONS(BoxCollider, Collider)

	public:
		BoxCollider(const Vector3& center, const Vector3& halfSize);
		~BoxCollider();

		void Tick(float deltaTime) override;

		bool Overlap(const Collider& other) const override;

	private:

		void Init();

	private:
		Vector3 halfSize;

		std::vector<Vector3> BoxSize;
	};
}

