#include "Component.h"

namespace Engine
{
	void Component::OnAdd()
	{

	}

	void Component::OnRemove()
	{

	}
	
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
	
	void Component::Initialize()
	{

	}

	void Component::DispatchBeginPlay()
	{
		if (lifecycleState != LifecycleState::Initialized)
		{
			return;
		}
		
		lifecycleState = LifecycleState::BeginningPlay;
		
		BeginPlay();
				
		lifecycleState = LifecycleState::HasBegunPlay;
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
