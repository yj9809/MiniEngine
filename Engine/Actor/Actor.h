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

		virtual void Start();
		virtual void Update(float deltaTime);
		virtual void Draw();

		void SetPosition(const Vector2& position);

		Vector2 GetPositionI() const;
		Vector2 GetPositionF() const;

		inline void SetOwner(Level* newOwner) { owner = newOwner; }
		inline Level* GetOwner() const { return owner; }

		inline void SetStarted() { isStarted = true; }
		inline bool IsStarted() const { return isStarted; }

		inline void Destroy() { isDestroyed = true; }
		inline bool IsDestroyed() const { return isDestroyed; }

	protected:
		bool isStarted = false;

		bool isDestroyed = false;

		std::string image;

		int width = 0;

		int height = 0;

		Level* owner = nullptr;

		Vector2 position;
	};
}

