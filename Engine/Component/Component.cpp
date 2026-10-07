#include "Component.h"

#include <cassert>

namespace Engine
{
	void Component::DispatchOnAdd(Actor& newOwner)
	{
		if(owner != nullptr)
		{
			 assert(false && "Component is already attached to an Actor.");
        	return;
		}

		owner = &newOwner;
		OnAdd();
	}

	void Component::OnAdd()
	{

	}

	void Component::DispatchOnRemove()
	{
		if(lifecycleState == LifecycleState::Initializing || lifecycleState == LifecycleState::BeginningPlay)
		{
			assert(false && "Remove during lifecycle transition is not supported.");
			return;
		}

		if(lifecycleState == LifecycleState::EndingPlay || lifecycleState == LifecycleState::HasEndedPlay)
		{
			return;
		}

		lifecycleState = LifecycleState::EndingPlay;
		
		UnregisterFromSystem();
		OnRemove();

		lifecycleState = LifecycleState::HasEndedPlay;
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
		
		RegisterWithSystem();
		BeginPlay();
				
		lifecycleState = LifecycleState::HasBegunPlay;
	}

	void Component::BeginPlay()
	{

	}

	void Component::Tick(float deltaTime)
	{

	}

	void Component::RegisterWithSystem()
	{

	}

	void Component::UnregisterFromSystem()
	{

	}
}
