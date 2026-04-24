#include "QuadActor.h"

#include "Renderer/RenderCommand.h"
#include "Core/Input.h"
#include "Core/Time.h"

#include <vector>

#include "Component/Transform/TransformComponent.h"

void QuadActor::Init(Engine::IRenderer* renderer)
{
    this->renderer = renderer;

    // Todo: 테스트를 위해 임시로 구조체를 만들어 정점 데이터 생성.
    struct Vertex
    {
        float x, y, z;
        float r, g, b, a;
    };

    // 중심(0) + 8각형 둘레 (1~8) — 삼각형 팬
    Vertex vertices[] =
    {
        { 0.00f,  0.00f, 0.0f, 0.5f, 0.5f, 0.5f, 1.0f}, // 0 중심
        { 0.00f,  0.50f, 0.0f, 1.0f, 0.2f, 0.2f, 1.0f}, // 1
        { 0.35f,  0.35f, 0.0f, 1.0f, 0.6f, 0.2f, 1.0f}, // 2
        { 0.50f,  0.00f, 0.0f, 0.8f, 1.0f, 0.2f, 1.0f}, // 3
        { 0.35f, -0.35f, 0.0f, 0.2f, 1.0f, 0.4f, 1.0f}, // 4
        { 0.00f, -0.50f, 0.0f, 0.2f, 0.8f, 1.0f, 1.0f}, // 5
        {-0.35f, -0.35f, 0.0f, 0.2f, 0.4f, 1.0f, 1.0f}, // 6
        {-0.50f,  0.00f, 0.0f, 0.6f, 0.2f, 1.0f, 1.0f}, // 7
        {-0.35f,  0.35f, 0.0f, 1.0f, 0.2f, 0.8f, 1.0f}, // 8
    };

    UINT indices[] =
    {
        0, 1, 2,
        0, 2, 3,
        0, 3, 4,
        0, 4, 5,
        0, 5, 6,
        0, 6, 7,
        0, 7, 8,
        0, 8, 1,
    };

    vertexBuffer = renderer->CreateVertexBuffer(vertices, sizeof(vertices));
    indexBuffer = renderer->CreateIndexBuffer(indices, sizeof(indices));
}

void QuadActor::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

    if (Engine::Input::GetKeyDown('1'))
        currentPass = Engine::RenderLayerType::Opaque;
    else if (Engine::Input::GetKeyDown('2'))
        currentPass = Engine::RenderLayerType::Wireframe;

    Engine::Vector3 rotation = rootComponent->GetLocalRotationEulerDeg();
    rotation.z += deltaTime * 180.0f; // 초당 45도 회전
    rootComponent->SetLocalRotationEulerDeg(rotation);
}

void QuadActor::Draw()
{
    Actor::Draw();
    Engine::RenderCommand command;
    command.vertexBuffer = vertexBuffer;
    command.indexBuffer = indexBuffer;
    command.indexCount = 24;
    command.stride = sizeof(float) * 7; // x, y, z, r, g, b, a
    command.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    command.worldMatrix = rootComponent->GetWorldMatrix();
    command.passType = currentPass;

    renderer->Submit(command);
}
