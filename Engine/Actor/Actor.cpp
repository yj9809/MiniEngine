#include "Actor.h"
namespace Engine
{
	Actor::Actor()
	{
	}
	Actor::~Actor()
	{
	}
	void Actor::Start()
	{
	}
	void Actor::Update(float deltaTime)
	{
		
	}
	void Actor::Draw()
	{
	}

	void Actor::SetPosition(const Vector2& position)
	{
		this->position = position;
	}

	Vector2 Actor::GetPositionI() const
	{
		return {static_cast<int>(position.x), static_cast<int>(position.y)};
	}

	Vector2 Actor::GetPositionF() const
	{
		return position;
	}
}