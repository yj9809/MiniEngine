#pragma once

#include "Actor/Actor.h"
#include "Renderer/IRenderer.h"

class TriangleActor : public Engine::Actor
{
    RTTI_DECLARATIONS(TriangleActor, Actor)
    
public:
    void Init(Engine::IRenderer* renderer);
    virtual void BeginPlay() override;
    virtual void Tick(float deltaTime) override;
    virtual void Draw() override;

private:
    Engine::IRenderer* renderer = nullptr;
    Engine::BufferHandle vertexBuffer = Engine::NULL_BUFFER;
    Engine::BufferHandle indexBuffer = Engine::NULL_BUFFER;
    float angle = 0.0f;
    Engine::RenderLayerType currentPass = Engine::RenderLayerType::Opaque;
};
