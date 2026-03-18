#include "Actor.h"
#include "Level/Level.h"

namespace Engine
{
	Actor::Actor()
	{
	}

	Actor::~Actor()
	{
	}

	void Actor::BeginPlay()
	{
	}

	void Actor::Tick(float deltaTime)
	{
		if (!components.empty())
		{
			for (auto& component : components)
			{
				component->Tick(deltaTime);
			}
		}
	}

	void Actor::Draw()
	{
	}

	void Actor::OnDestroy()
	{
		// Tick/Draw에서 즉시 제외하고, 다음 ProcessAddAndDestroyActor()에서 메모리 해제.
		isActive = false;
		destroyRequested = true;
	}

	void Actor::SetPosition(const Vector2& position)
	{
		this->position = position;
	}

	Vector2 Actor::GetPositionI() const
	{
		// 콘솔 렌더러는 정수 좌표를 요구하므로 float를 int로 잘라낸다(반올림 아님).
		// Vector2의 멤버가 float이므로, int로 truncate한 뒤 다시 float로 변환해 담는다.
		return Vector2(static_cast<float>(static_cast<int>(position.x)), static_cast<float>(static_cast<int>(position.y)));
	}

	Vector2 Actor::GetPositionF() const
	{
		return position;
	}
}
