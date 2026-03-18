#pragma once

#include "Common/RTTI.h"
#include "Math/Vector2.h"
#include "Component/Component.h"

#include <string>
#include <memory>
#include <vector>

namespace Engine
{
	class Level;

	class ENGINE_API Actor : public RTTI
	{
		RTTI_DECLARATIONS(Actor, RTTI)

	public:
		Actor();
		virtual ~Actor();

		virtual void BeginPlay();
		virtual void Tick(float deltaTime);
		virtual void Draw();

		virtual void OnDestroy();

		template <typename T>
		T* AddComponent()
		{
			auto newComponent = std::make_unique<T>();
			T* ptr = newComponent.get();
			ptr->owner = this;
			components.emplace_back(std::move(newComponent));
			ptr->OnAdd();
			return ptr;
		}

		// Getter/Setter.
		void SetPosition(const Vector2& position);

		Vector2 GetPositionI() const;
		Vector2 GetPositionF() const;

		inline void SetOwner(Level* newOwner) { owner = newOwner; }
		inline Level* GetOwner() const { return owner; }

		inline bool IsActive() const { return isActive; }
		inline bool IsDestroyRequested() const { return destroyRequested; }

	protected:
		bool isActive = true;

		bool destroyRequested = false;	

		std::string image;

		int width = 0;

		int height = 0;

		Level* owner = nullptr;

		Vector2 position;

	private:
		std::vector<std::unique_ptr<Component>> components;
	};
}

