#pragma once

#include "Actor/Actor.h"
#include "Component/Light/DirectionalLightComponent.h"

class DirectionalLightActor : public Engine::Actor
{
    RTTI_DECLARATIONS(DirectionalLightActor, Actor)

public:
    DirectionalLightActor();

    virtual void Initialize() override;
    virtual void Tick(float deltaTime) override;

private:
    Engine::DirectionalLightComponent* directionalLight = nullptr;
};