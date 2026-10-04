#pragma once

#include "Actor/Actor.h"
#include "Component/Camera/CameraComponent.h"

class CameraActor : public Engine::Actor
{
    RTTI_DECLARATIONS(CameraActor, Actor)
    
public:
    CameraActor();
    
    virtual void BeginPlay() override;
    virtual void Tick(float deltaTime) override;
private:
    Engine::CameraComponent* mainCamera;
};
