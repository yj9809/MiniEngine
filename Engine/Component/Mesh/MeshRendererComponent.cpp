#include "MeshRendererComponent.h"

#include <utility>

#include "Actor/Actor.h"
#include "Component/Transform/TransformComponent.h"
#include "Level/Level.h"
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

    bool MeshRendererComponent::BuildRenderCommand(const Matrix4& viewMatrix, const Matrix4& projectionMatrix, RenderCommand& outCommand) const
    {
        if (!mesh)
        {
            return false;
        }

        outCommand = RenderCommand();

        outCommand.vertexBuffer = mesh->GetVertexBuffer();
        outCommand.indexBuffer = mesh->GetIndexBuffer();
        outCommand.indexCount = mesh->GetIndexCount();
        outCommand.stride = mesh->GetStride();
        outCommand.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

        if (!materials.empty() && materials[0])
        {
            const auto& materialTexture = materials[0]->GetMainTexture();

            outCommand.texture = materialTexture ? materialTexture->GetTextureHandle() : NULL_TEXTURE;
            outCommand.baseColor = materials[0]->GetBaseColor();
        }

        outCommand.worldMatrix = GetOwner().GetRootComponent()->GetWorldMatrix();
        outCommand.viewMatrix = viewMatrix;
        outCommand.projectionMatrix = projectionMatrix;
        outCommand.layerType = layerType;

        return true;
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
