#include "MeshRendererComponent.h"

#include "Actor/Actor.h"
#include "Component/Camera/CameraComponent.h"
#include "Component/Transform/TransformComponent.h"
#include "Level/Level.h"
#include "Renderer/IRenderer.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"

namespace Engine
{
    void MeshRendererComponent::SetMesh(std::shared_ptr<Mesh> newMesh)
    {
        mesh = newMesh;
    }

    void MeshRendererComponent::SetTexture(std::shared_ptr<Texture> newTexture)
    {
        texture = newTexture;
    }

    void MeshRendererComponent::SetLayerType(RenderLayerType newLayerType)
    {
        layerType = newLayerType;
    }

    void MeshRendererComponent::Initialize(IRenderer* renderer)
    {
        this->renderer = renderer;
        
        indexCount = mesh->GetIndexCount();
        stride = mesh->GetStride();
        
        vertexBuffer = mesh->GetVertexBuffer();
        indexBuffer = mesh->GetIndexBuffer();
    }

    void MeshRendererComponent::Draw()
    {
        Component::Draw();
        
        Matrix4 worldMatrix = GetOwner().GetRootComponent()->GetWorldMatrix();
        Matrix4 viewMatrix = GetOwner().GetOwner()->GetMainCamera()->GetViewMatrix();
        Matrix4 projectionMatrix = GetOwner().GetOwner()->GetMainCamera()->GetProjectionMatrix();
        
        renderCommand.vertexBuffer = vertexBuffer;
        renderCommand.indexBuffer = indexBuffer;
        renderCommand.texture = texture ? texture->GetTextureHandle() : NULL_TEXTURE;
        renderCommand.indexCount = indexCount;
        renderCommand.stride = stride;
        renderCommand.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        renderCommand.worldMatrix = worldMatrix;
        renderCommand.viewMatrix = viewMatrix;
        renderCommand.projectionMatrix = projectionMatrix;
        renderCommand.layerType = layerType;
        
        renderer->Submit(renderCommand);
    }
}
