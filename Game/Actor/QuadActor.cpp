#include "QuadActor.h"

#include "Renderer/RenderCommand.h"

#include <vector>

void QuadActor::Init(Engine::IRenderer* renderer)
{
    this->renderer = renderer;
    
    // Todo: 테스트를 위해 임시로 구조체를 만들어 정점 데이터 생성.
    struct Vertex
    {
        float x, y, z;
        float r, g, b, a;
    };
    
    Vertex vertices[] = 
    {
        {-0.5f,  0.5f, 0.0f,  0.5f, 0.5f, 0.5f, 1.0f},  // 0 좌상 (빨강)
        { 0.5f,  0.5f, 0.0f,  0.5f, 1.0f, 0.5f, 1.0f},  // 1 우상 (초록)
        { 0.5f, -0.5f, 0.0f,  0.5f, 0.5f, 1.0f, 1.0f},  // 2 우하 (파랑)
        {-0.5f, -0.5f, 0.0f,  1.0f, 1.0f, 0.5f, 1.0f},
    };
    
    UINT indices[] = {0, 1, 2, 2, 3, 0};
    
    vertexBuffer = renderer->CreateVertexBuffer(vertices, sizeof(vertices));
    indexBuffer = renderer->CreateIndexBuffer(indices, sizeof(indices));
}

void QuadActor::Draw()
{
    Actor::Draw();
    
    Engine::RenderCommand command;
    command.vertexBuffer = vertexBuffer;
    command.indexBuffer = indexBuffer;
    command.indexCount = 6;
    command.stride = sizeof(float) * 7; // x, y, z, r, g, b, a
    command.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    
    renderer->Submit(command);
}
