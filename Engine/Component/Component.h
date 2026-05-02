#pragma once

#include "Common/RTTI.h"

namespace Engine
{
	class Actor;

	class ENGINE_API Component : public RTTI
	{
		RTTI_DECLARATIONS(Component, RTTI)
		friend class Actor;

	public:
		virtual ~Component() = default;

		virtual void OnAdd();
		virtual void OnRemove();
		virtual void Tick(float deltaTime);
		virtual void Draw();

		Actor& GetOwner() const { return *owner; }

	private:
		Actor* owner = nullptr;
	};
}

