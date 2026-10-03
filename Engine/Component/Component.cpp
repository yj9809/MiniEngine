#include "Component.h"

namespace Engine
{
	void Component::DispatchInitialize()
	{
		if (lifecycleState != LifecycleState::Constructed)
		{
			return;
		}
		
		lifecycleState = LifecycleState::Initializing;

		Initialize();

		lifecycleState = LifecycleState::Initialized;
	}

	void Component::OnAdd()
	{

	}

	void Component::OnRemove()
	{

	}

	void Component::Initialize()
	{

	}

	void Component::BeginPlay()
	{

	}

	void Component::Tick(float deltaTime)
	{

	}

	void Component::Draw()
	{
		
	}
}
