#pragma once

#include "Actor/Actor.h"
#include "Renderer/IRenderer.h"

class QuadActor : public Engine::Actor
{
public:
    void Init(Engine::IRenderer* renderer);
    virtual void Tick(float deltaTime) override;
    virtual void Draw() override;
    
private:
    Engine::IRenderer* renderer = nullptr;
    Engine::BufferHandle vertexBuffer = Engine::NULL_BUFFER;
    Engine::BufferHandle indexBuffer = Engine::NULL_BUFFER;    
    float angle = 0.0f;
};
