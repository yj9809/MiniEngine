#pragma once

namespace Engine
{
	class Actor;

	class Component
	{
		friend class Actor;

	public:
		virtual void OnAdd();
		virtual void OnRemove();

		Actor& GetOwner() const { return *owner; }

	private:
		Actor* owner = nullptr;
	};
}

