#include "Component.h"

#include <cassert>
#include "Common/CleanupExceptionCollector.h"

namespace Engine
{
    void Component::DispatchOnAdd(Actor& newOwner)
    {
        if (owner != nullptr)
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
        if (lifecycleState == LifecycleState::Initializing || lifecycleState == LifecycleState::BeginningPlay)
        {
            assert(false && "Remove during lifecycle transition is not supported.");
            return;
        }

        if (lifecycleState == LifecycleState::EndingPlay || lifecycleState == LifecycleState::HasEndedPlay)
        {
            return;
        }

        CleanupExceptionCollector cleanup;

        lifecycleState = LifecycleState::EndingPlay;

        cleanup.Try([&]()
        {
            UnregisterFromSystem();
        });

        cleanup.Try([&]()
        {
            OnRemove();
        });

        lifecycleState = LifecycleState::HasEndedPlay;

        cleanup.RethrowIfAny();
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

        try
        {
            Initialize();
        }
        catch (...)
        {
            // 전이 상태에서 벗어나 DispatchOnRemove()가 정리할 수 있게 한다.
            lifecycleState = LifecycleState::InitializationFailed;
            throw;
        }

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

        try
        {
            RegisterWithSystem();
            BeginPlay();
        }
        catch (...)
        {
            // 등록이 일부 진행됐을 수 있으므로 제거 가능한 상태로 만든다.
            lifecycleState = LifecycleState::BeginPlayFailed;
            throw;
        }

        lifecycleState = LifecycleState::HasBegunPlay;
    }

    void Component::BeginPlay()
    {
    }

    bool Component::IsEndingOrEnded() const
    {
        return lifecycleState == LifecycleState::EndingPlay || lifecycleState == LifecycleState::HasEndedPlay;
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
