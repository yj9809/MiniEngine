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
#include "Component/Light/DirectionalLightComponent.h"
#include "Component/Light/SkyLightComponent.h"
#include "Component/Transform/TransformComponent.h"
#include "Component/Mesh/MeshRendererComponent.h"
#include "Level/Level.h"
#include "Renderer/IRenderer.h"
#include "Renderer/Mesh.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/GPUData/LightingPacking.h"
#include "Renderer/RenderData/RenderFrameData.h"
#include "Resource/ResourceManager.h"
#include "System/RenderingSystem.h"
#include "System/LightingSystem.h"

namespace
{
    class RecordingRenderer final : public Engine::IRenderer
    {
    public:
        Engine::RendererInitResult GPUInit(HWND, int, int) override { return std::nullopt; }

        void Submit(const Engine::RenderCommand& command) override
        {
            submittedCommands.push_back(command);
        }

        void SubmitDirectionalLights(
            const std::vector<Engine::DirectionalLightRenderData>& lights) override
        {
            submittedDirectionalLights.assign(lights.begin(), lights.end());
        }

        void SubmitEnvironmentLighting(
            const Engine::EnvironmentLightingRenderData& lighting) override
        {
            submittedEnvironmentLighting = lighting;
            ++environmentSubmissions;
        }

        void SubmitCameraPosition(const Engine::Vector3& position) override
        {
            submittedCameraPosition = position;
            ++cameraSubmissions;
        }
        Engine::Vector3 submittedCameraPosition{};
        int cameraSubmissions = 0;

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
        Engine::EnvironmentLightingRenderData submittedEnvironmentLighting{};
        int environmentSubmissions = 0;
        std::vector<Engine::BufferHandle> releasedBuffers;
        std::vector<Engine::TextureHandle> releasedTextures;

    private:
        Engine::BufferHandle nextBufferHandle = 1;
        Engine::TextureHandle nextTextureHandle = 1;
    };

    void ExpectColor(const Engine::Vector3& actual, const Engine::Vector3& expected)
    {
        EXPECT_FLOAT_EQ(actual.x, expected.x);
        EXPECT_FLOAT_EQ(actual.y, expected.y);
        EXPECT_FLOAT_EQ(actual.z, expected.z);
    }

    void ExpectEnvironment(const RecordingRenderer& renderer,
        const Engine::Vector3& sky, const Engine::Vector3& ground)
    {
        SCOPED_TRACE("Submitted environment lighting");
        ExpectColor(renderer.submittedEnvironmentLighting.skyContribution, sky);
        ExpectColor(renderer.submittedEnvironmentLighting.groundContribution, ground);
    }

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

    class SkyActor final : public Engine::Actor
    {
    public:
        void DestroyForTest() { DispatchOnDestroy(); }
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

    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, nullptr);

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

    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, nullptr);
    actorPtr->DestroyForTest();
    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, nullptr);
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
    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, nullptr);
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

    firstLevel.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, nullptr);
    secondLevel.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, nullptr);

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

    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, nullptr);

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
    auto* cameraActorPtr = cameraActor.get();
    cameraActor->SetPosition(Engine::Vector3(3, -2, 5));

    ASSERT_NE(mesh, nullptr);
    level.AttachServices(renderer, resourceManager);
    level.SetMainCamera(camera);
    level.AddNewActor(std::move(cameraActor));
    level.AddNewActor(std::make_unique<MeshActor>(mesh));
    level.ProcessAddAndDestroyActor();

    level.Draw();

    EXPECT_EQ(renderer.submittedCommands.size(), 1);
    ExpectColor(renderer.submittedCameraPosition, Engine::Vector3(3, -2, 5));
    EXPECT_EQ(renderer.cameraSubmissions, 1);
    cameraActorPtr->SetPosition(Engine::Vector3(-4, 1, 2));
    level.Draw();
    ExpectColor(renderer.submittedCameraPosition, Engine::Vector3(-4, 1, 2));
    EXPECT_EQ(renderer.cameraSubmissions, 2);
    ASSERT_FALSE(renderer.submittedCommands.empty());
    const auto eyeInView = renderer.submittedCommands.back().viewMatrix.TransformPosition(
        renderer.submittedCameraPosition);
    ExpectColor(eyeInView, Engine::Vector3::zero);
    level.EndLevel();
}

TEST(RenderingSystemTest, EnvironmentIsSubmittedWithoutMeshesAlongsideDirectionalLight)
{
    RecordingRenderer renderer;
    Engine::RenderingSystem rendering(renderer);
    Engine::SkyLightComponent first, second;
    Engine::Actor directionalActor;
    auto* directional = directionalActor.AddComponent<Engine::DirectionalLightComponent>();
    Engine::LightingSystem lighting;
    first.SetSkyColor(Engine::Vector3(0.25f, 0.5f, 2.0f));
    first.SetGroundColor(Engine::Vector3(3.0f, 0.75f, 0.125f));
    first.SetIntensity(2.0f);
    second.SetSkyColor(Engine::Vector3(2.0f, 0.0f, 1.0f));
    second.SetGroundColor(Engine::Vector3(0.0f, 4.0f, 2.0f));
    second.SetIntensity(0.5f);
    directional->SetColor(Engine::Vector3(0.2f, 0.4f, 0.8f));
    directional->SetIntensity(3.0f);
    lighting.Register(&first);
    lighting.Register(&second);
    lighting.Register(directional);

    rendering.Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, &lighting);

    EXPECT_EQ(renderer.environmentSubmissions, 1);
    EXPECT_TRUE(renderer.submittedCommands.empty());
    ExpectEnvironment(renderer, Engine::Vector3(1.5f, 1.0f, 4.5f),
        Engine::Vector3(6.0f, 3.5f, 1.25f));
    ASSERT_EQ(renderer.submittedDirectionalLights.size(), 1u);
    ExpectColor(renderer.submittedDirectionalLights[0].color, Engine::Vector3(0.2f, 0.4f, 0.8f));
    EXPECT_FLOAT_EQ(renderer.submittedDirectionalLights[0].intensity, 3.0f);
}

TEST(RenderingSystemTest, NullAndEmptyLightingOverwritePreviousEnvironment)
{
    RecordingRenderer renderer;
    Engine::RenderingSystem rendering(renderer);
    Engine::SkyLightComponent sky;
    Engine::LightingSystem populated, empty;
    populated.Register(&sky);

    rendering.Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, nullptr);
    EXPECT_EQ(renderer.environmentSubmissions, 1);
    ExpectEnvironment(renderer, Engine::Vector3::zero, Engine::Vector3::zero);

    rendering.Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, &populated);
    ExpectEnvironment(renderer, Engine::Vector3(0.1f, 0.1f, 0.1f), Engine::Vector3(0.1f, 0.1f, 0.1f));
    rendering.Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, nullptr);
    EXPECT_EQ(renderer.environmentSubmissions, 3);
    ExpectEnvironment(renderer, Engine::Vector3::zero, Engine::Vector3::zero);

    rendering.Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, &populated);
    ExpectEnvironment(renderer, Engine::Vector3(0.1f, 0.1f, 0.1f), Engine::Vector3(0.1f, 0.1f, 0.1f));
    rendering.Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, &empty);
    EXPECT_EQ(renderer.environmentSubmissions, 5);
    ExpectEnvironment(renderer, Engine::Vector3::zero, Engine::Vector3::zero);
}

TEST(RenderingSystemTest, EachRenderReflectsChangesAndUnregistrationWithoutAccumulating)
{
    RecordingRenderer renderer;
    Engine::RenderingSystem rendering(renderer);
    Engine::SkyLightComponent sky;
    Engine::LightingSystem lighting;
    lighting.Register(&sky);

    rendering.Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, &lighting);
    ExpectEnvironment(renderer, Engine::Vector3(0.1f, 0.1f, 0.1f), Engine::Vector3(0.1f, 0.1f, 0.1f));
    sky.SetSkyColor(Engine::Vector3(2.0f, 3.0f, 4.0f));
    sky.SetGroundColor(Engine::Vector3(6.0f, 2.0f, 1.0f));
    sky.SetIntensity(0.5f);
    for (int frame = 0; frame < 2; ++frame)
    {
        rendering.Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, &lighting);
        ExpectEnvironment(renderer, Engine::Vector3(1.0f, 1.5f, 2.0f), Engine::Vector3(3.0f, 1.0f, 0.5f));
    }

    lighting.Unregister(&sky);
    rendering.Render(Engine::Matrix4::identity, Engine::Matrix4::identity, Engine::Vector3::zero, &lighting);
    EXPECT_EQ(renderer.environmentSubmissions, 4);
    ExpectEnvironment(renderer, Engine::Vector3::zero, Engine::Vector3::zero);
}

TEST(RenderingSystemTest, LevelDrawForwardsEnvironmentAndReflectsActorRemoval)
{
    RecordingRenderer renderer;
    Engine::ResourceManager resources(renderer);
    Engine::Level level;
    level.AttachServices(renderer, resources);
    auto cameraActor = std::make_unique<CameraActor>();
    level.SetMainCamera(cameraActor->GetCamera());
    level.AddNewActor(std::move(cameraActor));
    auto skyActor = std::make_unique<SkyActor>();
    auto* skyActorPtr = skyActor.get();
    auto* sky = skyActor->AddComponent<Engine::SkyLightComponent>();
    sky->SetSkyColor(Engine::Vector3(2.0f, 4.0f, 6.0f));
    sky->SetGroundColor(Engine::Vector3(8.0f, 2.0f, 0.0f));
    sky->SetIntensity(0.5f);
    level.AddNewActor(std::move(skyActor));
    level.ProcessAddAndDestroyActor();

    level.Draw();
    EXPECT_EQ(renderer.environmentSubmissions, 1);
    ExpectEnvironment(renderer, Engine::Vector3(1.0f, 2.0f, 3.0f), Engine::Vector3(4.0f, 1.0f, 0.0f));

    skyActorPtr->DestroyForTest();
    level.ProcessAddAndDestroyActor();
    level.Draw();
    EXPECT_EQ(renderer.environmentSubmissions, 2);
    ExpectEnvironment(renderer, Engine::Vector3::zero, Engine::Vector3::zero);
    level.EndLevel();
}

TEST(RenderingSystemTest, QuaternionLightDirectionReachesSubmissionAndPackingInWorldSpace)
{
    using namespace Engine;
    RecordingRenderer renderer;
    RenderingSystem rendering(renderer);
    Actor actor;
    auto* light = actor.AddComponent<DirectionalLightComponent>();
    light->SetColor(Vector3(0.25f, 0.5f, 2.0f));
    light->SetIntensity(3.0f);
    LightingSystem lighting;
    lighting.Register(light);
    const Quaternion rotations[] = {Quaternion(), Quaternion(1, 1, 1, 1), Quaternion(0, 1, 0, 0)};
    const Vector3 expected[] = {Vector3::unitX, Vector3::unitY, Vector3(-1, 0, 0)};
    for (int pose = 0; pose < 3; ++pose)
    {
        ASSERT_TRUE(actor.GetRootComponent()->TrySetLocalRotation(rotations[pose]));
        // Vary the camera view too: lighting must remain in world space.
        rendering.Render(Matrix4::Rotation(0, 0, pose * PI * 0.5f), Matrix4::identity, Engine::Vector3::zero, &lighting);
        ASSERT_EQ(renderer.submittedDirectionalLights.size(), 1u);
        const auto& submitted = renderer.submittedDirectionalLights[0];
        EXPECT_NEAR(submitted.direction.x, expected[pose].x, 1e-6f);
        EXPECT_NEAR(submitted.direction.y, expected[pose].y, 1e-6f);
        EXPECT_NEAR(submitted.direction.z, expected[pose].z, 1e-6f);
        ExpectColor(submitted.color, Vector3(0.25f, 0.5f, 2.0f));
        EXPECT_FLOAT_EQ(submitted.intensity, 3.0f);
        RenderFrameData frame;
        frame.directionalLights = renderer.submittedDirectionalLights;
        const auto packed = BuildLightingConstantBuffer(frame);
        ASSERT_EQ(packed.directionalLightCount, 1u);
        const auto& direction = packed.directionalLights[0].direction;
        EXPECT_NEAR(direction.x, expected[pose].x, 1e-6f);
        EXPECT_NEAR(direction.y, expected[pose].y, 1e-6f);
        EXPECT_NEAR(direction.z, expected[pose].z, 1e-6f);
        EXPECT_FLOAT_EQ(direction.w, 0.0f);
    }
    lighting.Unregister(light);
    rendering.Render(Matrix4::identity, Matrix4::identity, Engine::Vector3::zero, &lighting);
    EXPECT_TRUE(renderer.submittedDirectionalLights.empty());
}
