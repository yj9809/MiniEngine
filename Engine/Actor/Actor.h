#pragma once

#include "Common/RTTI.h"
#include "Math/Vector2.h"

#include <string>

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

		// Getter/Setter.
		void SetPosition(const Vector2& position);

		Vector2 GetPositionI() const;
		Vector2 GetPositionF() const;

		inline void SetOwner(Level* newOwner) { owner = newOwner; }
		inline Level* GetOwner() const { return owner; }

	protected:
		bool isDestroyed = false;
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
	};
}

