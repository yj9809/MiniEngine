#include "TriangleActor.h"

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

void TriangleActor::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

    if (Engine::Input::GetKeyDown('1'))
        currentPass = Engine::RenderLayerType::Opaque;
    else if (Engine::Input::GetKeyDown('2'))
        currentPass = Engine::RenderLayerType::Wireframe;

    angle += deltaTime * -1.0f;
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
    command.worldMatrix = Engine::Matrix4::RotationZ(angle);
    command.passType = currentPass;

    renderer->Submit(command);
}
