#include "Actor.h"
#include "Component/Transform/TransformComponent.h"

namespace Engine
{
    Actor::Actor()
    {
        rootComponent = AddComponent<TransformComponent>();
    }

    Actor::~Actor()
    {
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

    void Actor::Draw()
    {
    }

    void Actor::OnDestroy()
    {
        // Tick/Draw에서 즉시 제외하고, 다음 ProcessAddAndDestroyActor()에서 메모리 해제.
        isActive = false;
        destroyRequested = true;
        for (auto& component : components)
        {
            component->OnRemove();
        }
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
}
