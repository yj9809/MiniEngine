#include "MeshRendererComponent.h"

#include <utility>

#include "Actor/Actor.h"
#include "Component/Camera/CameraComponent.h"
#include "Component/Transform/TransformComponent.h"
#include "Level/Level.h"
#include "Renderer/IRenderer.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include "Renderer/Material.h"
#include "System/RenderingSystem.h"

namespace Engine
{
    void MeshRendererComponent::SetMesh(std::shared_ptr<Mesh> newMesh)
    {
        mesh = newMesh;
    }

    void MeshRendererComponent::SetMaterial(size_t slot, std::shared_ptr<Material> newMaterial)
    {
        if (materials.size() <= slot)
        {
            materials.resize(slot + 1);
        }
        materials[slot] = std::move(newMaterial);
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

        // 기존 Texture를 사용하지 않고 MaterialTexture를 사용.     
        if(!materials.empty() && materials[0])
        {
            const auto& materialTexture = materials[0]->GetMainTexture();

            renderCommand.texture = materialTexture ? materialTexture->GetTextureHandle() : NULL_TEXTURE;
            renderCommand.baseColor = materials[0]->GetBaseColor();
        }
        else
        {
            renderCommand.texture = NULL_TEXTURE;
            renderCommand.baseColor = Vector4::one;
        }

        renderCommand.indexCount = indexCount;
        renderCommand.stride = stride;
        renderCommand.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        renderCommand.worldMatrix = worldMatrix;
        renderCommand.viewMatrix = viewMatrix;
        renderCommand.projectionMatrix = projectionMatrix;
        renderCommand.layerType = layerType;
        
        renderer->Submit(renderCommand);
    }

    void MeshRendererComponent::BeginPlay()
    {
        Component::BeginPlay();

        if(registeredSystem)
        {
            return;
        }

        Level* ownerLevel = GetOwner().GetOwner();

        if(!ownerLevel)
        {
            return;
        }

        RenderingSystem& system = ownerLevel->GetRenderingSystem();
        system.Register(this);
        registeredSystem = &system;
    }

    void MeshRendererComponent::OnRemove()
    {
        if(registeredSystem)
        {
            registeredSystem->Unregister(this);
            registeredSystem = nullptr;
        }

        Component::OnRemove();
    }
}
