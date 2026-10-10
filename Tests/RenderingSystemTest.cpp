#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "Actor/Actor.h"
#include "Component/Camera/CameraComponent.h"
#include "Component/Mesh/MeshRendererComponent.h"
#include "Level/Level.h"
#include "Renderer/IRenderer.h"
#include "Renderer/Mesh.h"
#include "Renderer/RenderCommand.h"
#include "Resource/ResourceManager.h"
#include "System/RenderingSystem.h"

namespace
{
    class RecordingRenderer final : public Engine::IRenderer
    {
    public:
        bool GPUInit(HWND, int, int) override { return true; }

        void Submit(const Engine::RenderCommand& command) override
        {
            submittedCommands.push_back(command);
        }

        void SubmitDirectionalLights(
            const std::vector<Engine::DirectionalLightRenderData>& lights) override
        {
            submittedDirectionalLights.assign(lights.begin(), lights.end());
        }

        void BeginFrame(float, float, float) override {}
        void EndFrame() override {}
        void Render() override {}

        Engine::BufferHandle CreateVertexBuffer(const void*, UINT) override
        {
            return nextBufferHandle++;
        }

        Engine::BufferHandle CreateIndexBuffer(const void*, UINT) override
        {
            return nextBufferHandle++;
        }

        void ReleaseBuffer(Engine::BufferHandle buffer) override
        {
            releasedBuffers.push_back(buffer);
        }

        Engine::TextureHandle CreateTexture(const wchar_t*) override
        {
            return nextTextureHandle++;
        }

        void ReleaseTexture(Engine::TextureHandle texture) override
        {
            releasedTextures.push_back(texture);
        }

    public:
        std::vector<Engine::RenderCommand> submittedCommands;
        std::vector<Engine::DirectionalLightRenderData> submittedDirectionalLights;
        std::vector<Engine::BufferHandle> releasedBuffers;
        std::vector<Engine::TextureHandle> releasedTextures;

    private:
        Engine::BufferHandle nextBufferHandle = 1;
        Engine::TextureHandle nextTextureHandle = 1;
    };

    class TemporaryTriangleObj final
    {
    public:
        TemporaryTriangleObj()
        {
            const auto uniqueName = "MiniEngine_RenderingSystemTest_" +
                std::to_string(reinterpret_cast<std::uintptr_t>(this)) + ".obj";
            path = std::filesystem::temp_directory_path() / uniqueName;

            std::ofstream file(path);
            file << "v 0 0 0\n"
                 << "v 1 0 0\n"
                 << "v 0 1 0\n"
                 << "vt 0 0\n"
                 << "vt 1 0\n"
                 << "vt 0 1\n"
                 << "vn 0 0 1\n"
                 << "f 1/1/1 2/2/1 3/3/1\n";
        }

        ~TemporaryTriangleObj()
        {
            std::error_code error;
            std::filesystem::remove(path, error);
        }

        std::string GetPath() const
        {
            return path.string();
        }

    private:
        std::filesystem::path path;
    };

    class MeshActor final : public Engine::Actor
    {
    public:
        explicit MeshActor(std::shared_ptr<Engine::Mesh> mesh = nullptr)
        {
            meshRenderer = AddComponent<Engine::MeshRendererComponent>();
            meshRenderer->SetMesh(std::move(mesh));
        }

        void DestroyForTest()
        {
            DispatchOnDestroy();
        }

        Engine::MeshRendererComponent* GetMeshRenderer() const
        {
            return meshRenderer;
        }

    private:
        Engine::MeshRendererComponent* meshRenderer = nullptr;
    };

    class CameraActor final : public Engine::Actor
    {
    public:
        CameraActor()
        {
            camera = AddComponent<Engine::CameraComponent>();
        }

        Engine::CameraComponent* GetCamera() const
        {
            return camera;
        }

    private:
        Engine::CameraComponent* camera = nullptr;
    };
}

TEST(RenderingSystemTest, RegisteredComponentSubmitsOnceAndDuplicateRegistrationIsIgnored)
{
    RecordingRenderer renderer;
    Engine::ResourceManager resourceManager(renderer);
    Engine::Level level;
    TemporaryTriangleObj obj;
    const auto mesh = resourceManager.LoadMesh(obj.GetPath());
    auto actor = std::make_unique<MeshActor>(mesh);
    MeshActor* actorPtr = actor.get();

    ASSERT_NE(mesh, nullptr);
    level.AttachServices(renderer, resourceManager);
    level.AddNewActor(std::move(actor));
    level.ProcessAddAndDestroyActor();
    level.GetRenderingSystem().Register(actorPtr->GetMeshRenderer());

    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, nullptr);

    EXPECT_EQ(renderer.submittedCommands.size(), 1);
    level.EndLevel();
}

TEST(RenderingSystemTest, DestroyedActorIsUnregisteredBeforeMemoryRelease)
{
    RecordingRenderer renderer;
    Engine::ResourceManager resourceManager(renderer);
    Engine::Level level;
    TemporaryTriangleObj obj;
    const auto mesh = resourceManager.LoadMesh(obj.GetPath());
    auto actor = std::make_unique<MeshActor>(mesh);
    MeshActor* actorPtr = actor.get();

    ASSERT_NE(mesh, nullptr);
    level.AttachServices(renderer, resourceManager);
    level.AddNewActor(std::move(actor));
    level.ProcessAddAndDestroyActor();

    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, nullptr);
    actorPtr->DestroyForTest();
    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, nullptr);
    level.ProcessAddAndDestroyActor();

    EXPECT_EQ(renderer.submittedCommands.size(), 1);
    level.EndLevel();
}

TEST(RenderingSystemTest, ClearBeforeComponentRemovalKeepsUnregisterSafe)
{
    RecordingRenderer renderer;
    Engine::ResourceManager resourceManager(renderer);
    Engine::Level level;
    TemporaryTriangleObj obj;
    const auto mesh = resourceManager.LoadMesh(obj.GetPath());
    auto actor = std::make_unique<MeshActor>(mesh);
    MeshActor* actorPtr = actor.get();

    ASSERT_NE(mesh, nullptr);
    level.AttachServices(renderer, resourceManager);
    level.AddNewActor(std::move(actor));
    level.ProcessAddAndDestroyActor();

    level.GetRenderingSystem().Clear();
    actorPtr->DestroyForTest();
    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, nullptr);
    level.ProcessAddAndDestroyActor();

    EXPECT_TRUE(renderer.submittedCommands.empty());
    level.EndLevel();
}

TEST(RenderingSystemTest, ComponentRegistersOnlyWithItsOwnerLevel)
{
    RecordingRenderer firstRenderer;
    RecordingRenderer secondRenderer;
    Engine::ResourceManager firstResourceManager(firstRenderer);
    Engine::ResourceManager secondResourceManager(secondRenderer);
    Engine::Level firstLevel;
    Engine::Level secondLevel;
    TemporaryTriangleObj obj;
    const auto mesh = firstResourceManager.LoadMesh(obj.GetPath());

    ASSERT_NE(mesh, nullptr);
    firstLevel.AttachServices(firstRenderer, firstResourceManager);
    secondLevel.AttachServices(secondRenderer, secondResourceManager);
    firstLevel.AddNewActor(std::make_unique<MeshActor>(mesh));
    firstLevel.ProcessAddAndDestroyActor();

    firstLevel.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, nullptr);
    secondLevel.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, nullptr);

    EXPECT_EQ(firstRenderer.submittedCommands.size(), 1);
    EXPECT_TRUE(secondRenderer.submittedCommands.empty());
    firstLevel.EndLevel();
    secondLevel.EndLevel();
}

TEST(RenderingSystemTest, ComponentWithoutMeshDoesNotSubmit)
{
    RecordingRenderer renderer;
    Engine::ResourceManager resourceManager(renderer);
    Engine::Level level;

    level.AttachServices(renderer, resourceManager);
    level.AddNewActor(std::make_unique<MeshActor>());
    level.ProcessAddAndDestroyActor();

    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, nullptr);

    EXPECT_TRUE(renderer.submittedCommands.empty());
    level.EndLevel();
}

TEST(RenderingSystemTest, LevelDrawSubmitsThroughRenderingSystem)
{
    RecordingRenderer renderer;
    Engine::ResourceManager resourceManager(renderer);
    Engine::Level level;
    TemporaryTriangleObj obj;
    const auto mesh = resourceManager.LoadMesh(obj.GetPath());
    auto cameraActor = std::make_unique<CameraActor>();
    Engine::CameraComponent* camera = cameraActor->GetCamera();

    ASSERT_NE(mesh, nullptr);
    level.AttachServices(renderer, resourceManager);
    level.SetMainCamera(camera);
    level.AddNewActor(std::move(cameraActor));
    level.AddNewActor(std::make_unique<MeshActor>(mesh));
    level.ProcessAddAndDestroyActor();

    level.Draw();

    EXPECT_EQ(renderer.submittedCommands.size(), 1);
    level.EndLevel();
}
