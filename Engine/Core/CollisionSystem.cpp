#include "CollisionSystem.h"

#include "Component/Physics/Collider.h"

#include <algorithm>

namespace Engine
{
	void CollisionSystem::Register(Collider* collider)
	{
		this->colliders.emplace_back(collider);
	}

	void CollisionSystem::UnRegister(Collider* collider)
	{
		auto it = std::find(colliders.begin(), colliders.end(), collider);
		if (it != colliders.end())
		{
			std::iter_swap(it, colliders.end() - 1);
			colliders.pop_back();
		}
	}
}