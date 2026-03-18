#pragma once

namespace Engine
{
	class Actor;

	class Component
	{
		friend class Actor;

	public:
		virtual ~Component() = default;

		virtual void OnAdd();
		virtual void OnRemove();
		virtual void Tick(float deltaTime);

		Actor& GetOwner() const { return *owner; }

	private:
		Actor* owner = nullptr;
	};
}

