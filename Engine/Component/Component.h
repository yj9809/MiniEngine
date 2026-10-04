#pragma once

#include "Common/RTTI.h"
#include "Common/LifecycleState.h"

namespace Engine
{
	class Actor;

	class ENGINE_API Component : public RTTI
	{
		RTTI_DECLARATIONS(Component, RTTI)
		friend class Actor;

	public:
		virtual ~Component() = default;
		
		virtual void OnAdd();
		virtual void OnRemove();
		
		// 엔진 내부에서 초기화를 보장하기 위한 DispatchInitialize() 호출. 외부에서 직접 호출하지 말 것.
		void DispatchInitialize();
		// 컴포넌트가 액터에 부착될 때 호출된다. 초기화 로직을 여기에 구현한다.
		virtual void Initialize();

		// 엔진 내부에서 BeginPlay를 보장하기 위한 DispatchBeginPlay() 호출. 외부에서 직접 호출하지 말 것.
		void DispatchBeginPlay();
		// 컴포넌트가 초기화까지 끝나면 BeginPlay()가 호출된다. Tick()이 호출되기 전에 한 번만 호출된다.
		virtual void BeginPlay();
		
		virtual void Tick(float deltaTime);
		virtual void Draw();

		Actor& GetOwner() const { return *owner; }

	private:
		Actor* owner = nullptr;
		
		// 컴포넌트의 생명주기 상태.
		// Constructed -> Initializing -> Initialized -> BeginningPlay -> HasBegunPlay.
		LifecycleState lifecycleState = LifecycleState::Constructed;
	};
}

