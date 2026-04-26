#include "TriangleActor.h"

#include "Component/Transform/TransformComponent.h"
#include "Renderer/RenderCommand.h"
#include "Core/Input.h"

void TriangleActor::Init(Engine::IRenderer* renderer)
{
    this->renderer = renderer;

    struct Vertex
    {
        float x, y, z;
        float r, g, b, a;
    };

    Vertex vertices[] =
    {
        { 0.80f,  0.70f, 0.0f, 1.0f, 0.2f, 0.2f, 1.0f },
        { 0.80f, -0.20f, 0.0f, 0.2f, 1.0f, 0.2f, 1.0f },
        {0.50f, -0.20f, 0.0f, 0.2f, 0.2f, 1.0f, 1.0f },
    };

    UINT indices[] = { 0, 1, 2 };

    vertexBuffer = renderer->CreateVertexBuffer(vertices, sizeof(vertices));
    indexBuffer = renderer->CreateIndexBuffer(indices, sizeof(indices));
}

void TriangleActor::BeginPlay()
{
    Actor::BeginPlay();
    
    rootComponent->SetLocalPosition({ 0.0f, 0.3f, 0.0f });
}

void TriangleActor::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

    if (Engine::Input::GetKeyDown('1'))
        currentPass = Engine::RenderLayerType::Opaque;
    else if (Engine::Input::GetKeyDown('2'))
        currentPass = Engine::RenderLayerType::Wireframe;

    Engine::Vector3 rotation = rootComponent->GetLocalRotationEulerDeg();
    rotation.z += deltaTime * 30.0f;
    rootComponent->SetLocalRotationEulerDeg(rotation);
}

void TriangleActor::Draw()
{
    Actor::Draw();

    Engine::RenderCommand command;
    command.vertexBuffer = vertexBuffer;
    command.indexBuffer = indexBuffer;
    command.indexCount = 3;
    command.stride = sizeof(float) * 7;
    command.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    command.worldMatrix = rootComponent->GetWorldMatrix();
    command.passType = currentPass;

    renderer->Submit(command);
}
