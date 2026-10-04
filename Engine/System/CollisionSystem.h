#pragma once

#include <vector>


namespace Engine
{
	class Collider;

	class CollisionSystem
	{
	public:
		void Register(Collider* collider);
		void UnRegister(Collider* collider);

	private:
		std::vector<Collider*> colliders;
	};
}

