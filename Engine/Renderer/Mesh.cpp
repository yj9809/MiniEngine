#include "Mesh.h"

#include "IRenderer.h"

Engine::Mesh::~Mesh()
{
    renderer->ReleaseBuffer(vertexBuffer);
    renderer->ReleaseBuffer(indexBuffer);
}

Engine::Mesh* Engine::Mesh::LoadFromOBJ(const char* fileName)
{
}
