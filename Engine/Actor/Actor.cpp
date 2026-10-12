#include "Actor.h"

#include <cassert>

#include "Common/CleanupExceptionCollector.h"
#include "Component/Transform/TransformComponent.h"
#include "Level/Level.h"

namespace Engine
{
    Actor::Actor()
    {
        rootComponent = AddComponent<TransformComponent>();
    }

    Actor::~Actor()
    {
    }

    void Actor::DispatchInitialize()
    {
        if (lifecycleState != LifecycleState::Constructed)
        {
            return;
        }
        
        lifecycleState = LifecycleState::Initializing;

        try
        {
            Initialize();
        
            for (auto & component : components)
            {
                component->DispatchInitialize();
            }
        }
        catch (...)
        {
            // DispatchOnDestroy()가 Component 정리를 수행할 수 있게 한다.
            lifecycleState = LifecycleState::InitializationFailed;
            throw;
        }
        
        lifecycleState = LifecycleState::Initialized;
    }

    void Actor::Initialize()
    {
        
    }

    void Actor::DispatchBeginPlay()
    {
        if (lifecycleState != LifecycleState::Initialized)
        {
            return;
        }
        
        lifecycleState = LifecycleState::BeginningPlay;

        try
        {
            BeginPlay();
        
            for (auto& component : components)
            {
                component->DispatchBeginPlay();
            }
        }
        catch (...)
        {
            // BeginPlay가 일부 실행됐으므로 OnDestroy까지 수행할 수 있게 한다.
            lifecycleState = LifecycleState::BeginPlayFailed;
            throw;
        }
        
        lifecycleState = LifecycleState::HasBegunPlay;
    }

    void Actor::BeginPlay()
    {
        
    }

    void Actor::Tick(float deltaTime)
    {
        for (auto& component : components)
        {
            component->Tick(deltaTime);
        }
    }

    void Actor::DispatchOnDestroy()
    {
        if(lifecycleState == LifecycleState::Initializing || lifecycleState == LifecycleState::BeginningPlay)
        {
            assert(false && "Destroy during lifecycle transition is not supported.");
            return;
        }

        if(lifecycleState == LifecycleState::EndingPlay || lifecycleState == LifecycleState::HasEndedPlay)
        {
            return;
        }

        CleanupExceptionCollector cleanup;
        const bool shouldCallOnDestroy =
            lifecycleState == LifecycleState::HasBegunPlay ||
            lifecycleState == LifecycleState::BeginPlayFailed;

        lifecycleState = LifecycleState::EndingPlay;
        isActive = false;

        if (shouldCallOnDestroy)
        {
            cleanup.Try([&]()
            {
                OnDestroy();
            });
        }

        for (auto& component : components)
        {
            cleanup.Try([&]()
            {
                component->DispatchOnRemove();
            });
        }

        lifecycleState = LifecycleState::HasEndedPlay;
        cleanup.RethrowIfAny();
    }

    void Actor::OnDestroy()
    {

    }

    void Actor::SetPosition(const Vector3& position)
    {
        rootComponent->SetLocalPosition(position);
    }

    void Actor::SetRotation(const Vector3& rotation)
    {
        rootComponent->SetLocalRotationEulerDeg(rotation);
    }

    void Actor::SetScale(const Vector3& scale)
    {
        rootComponent->SetLocalScale(scale);
    }
    
    Vector3 Actor::GetPosition() const
    {
        return rootComponent->GetLocalPosition();
    }

    Vector3 Actor::GetRotation() const
    {
        return rootComponent->GetLocalRotationEulerDeg();
    }

    Vector3 Actor::GetScale() const
    {
        return rootComponent->GetLocalScale();
    }

    bool Actor::IsPendingDestroy() const
    {
        return lifecycleState == LifecycleState::HasEndedPlay;
    }
}
