#pragma once

#include "Common/RTTI.h"
#include "Common/LifecycleState.h"

namespace Engine
{
	class Actor;

	class ENGINE_API Component : public RTTI
	{
		RTTI_DECLARATIONS(Component, RTTI)

	public:
		virtual ~Component() = default;

		// 엔진 내부에서 부착을 보장하기 위한 DispatchOnAdd() 호출. 외부에서 직접 호출하지 말 것.
		void DispatchOnAdd(Actor& newOwner);

		// 엔진 내부에서 제거를 보장하기 위한 DispatchOnRemove() 호출. 외부에서 직접 호출하지 말 것.
		void DispatchOnRemove();
		
		// 엔진 내부에서 초기화를 보장하기 위한 DispatchInitialize() 호출. 외부에서 직접 호출하지 말 것.
		void DispatchInitialize();

		// 엔진 내부에서 BeginPlay를 보장하기 위한 DispatchBeginPlay() 호출. 외부에서 직접 호출하지 말 것.
		void DispatchBeginPlay();
		
		virtual void Tick(float deltaTime);

		Actor& GetOwner() const { return *owner; }

	protected:
		// Actor에 부착되는 즉시 호출된다.
    	// 이 시점에는 Actor의 Owner Level이 아직 없을 수 있다.
		virtual void OnAdd();

		// 컴포넌트가 액터에서 제거될 때 호출된다. 제거 로직을 여기에 구현한다.
		virtual void OnRemove();

		// Actor의 초기화 단계에서 호출된다. 초기화 로직을 여기에 구현한다.
		virtual void Initialize();
		
		// 컴포넌트가 초기화까지 끝나면 BeginPlay()가 호출된다. Tick()이 호출되기 전에 한 번만 호출된다.
		virtual void BeginPlay();	

	private:
		Actor* owner = nullptr;
		
		// 컴포넌트의 생명주기 상태.
		// Constructed -> Initializing -> Initialized
		// -> BeginningPlay -> HasBegunPlay
		// -> EndingPlay -> HasEndedPlay.
		LifecycleState lifecycleState = LifecycleState::Constructed;
	};
}

