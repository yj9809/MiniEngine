#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <vector>

#include "Actor/Actor.h"
#include "Component/Mesh/MeshRendererComponent.h"
#include "Level/Level.h"
#include "Renderer/IRenderer.h"
#include "Renderer/Material.h"
#include "Renderer/Mesh.h"
#include "Renderer/Texture.h"
#include "Resource/ResourceManager.h"
#include "System/RenderingSystem.h"

namespace
{
    // Tests ResourceManager/Mesh/Texture contracts, not real D3D or WIC failures.
    class ResourceProbeRenderer final : public Engine::IRenderer
    {
    public:
        ~ResourceProbeRenderer() override
        {
            EXPECT_TRUE(liveBuffers.empty());
            EXPECT_TRUE(liveTextures.empty());
        }
        Engine::RendererInitResult GPUInit(HWND, int, int) override { return std::nullopt; }
        void Submit(const Engine::RenderCommand& command) override
        {
            EXPECT_EQ(liveBuffers.count(command.vertexBuffer), 1u);
            EXPECT_EQ(liveBuffers.count(command.indexBuffer), 1u);
            EXPECT_EQ(liveTextures.count(command.texture), 1u);
            ++submissions;
        }
        void SubmitDirectionalLights(const std::vector<Engine::DirectionalLightRenderData>&) override {}
        void BeginFrame(float, float, float) override {}
        void EndFrame() override {}
        void Render() override {}
        Engine::BufferHandle CreateVertexBuffer(const void*, UINT) override
        {
            ++vertexCalls;
            return failVertex ? Engine::NULL_BUFFER : AllocateBuffer();
        }
        Engine::BufferHandle CreateIndexBuffer(const void*, UINT) override
        {
            ++indexCalls;
            return failIndex ? Engine::NULL_BUFFER : AllocateBuffer();
        }
        Engine::TextureHandle CreateTexture(const wchar_t*) override
        {
            ++textureCalls;
            if (failTexture) return Engine::NULL_TEXTURE;
            const auto handle = nextTexture++;
            liveTextures.insert(handle);
            return handle;
        }
        void ReleaseBuffer(Engine::BufferHandle handle) override
        {
            EXPECT_EQ(liveBuffers.erase(handle), 1u) << "Invalid or duplicate buffer release";
            ++bufferReleases;
        }
        void ReleaseTexture(Engine::TextureHandle handle) override
        {
            EXPECT_EQ(liveTextures.erase(handle), 1u) << "Invalid or duplicate texture release";
            ++textureReleases;
        }

        bool failVertex = false, failIndex = false, failTexture = false;
        int vertexCalls = 0, indexCalls = 0, textureCalls = 0;
        int bufferReleases = 0, textureReleases = 0, submissions = 0;
        std::set<Engine::BufferHandle> liveBuffers;
        std::set<Engine::TextureHandle> liveTextures;

    private:
        Engine::BufferHandle AllocateBuffer()
        {
            const auto handle = nextBuffer++;
            liveBuffers.insert(handle);
            return handle;
        }
        Engine::BufferHandle nextBuffer = 1;
        Engine::TextureHandle nextTexture = 1;
    };

    class ResourceManagerTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            static unsigned sequence = 0;
            directory = std::filesystem::temp_directory_path() /
                ("MiniEngine_ResourceTest_" + std::to_string(GetCurrentProcessId()) +
                    "_" + std::to_string(++sequence));
            // Only remove files from a directory this test actually created.
            createdDirectory = std::filesystem::create_directory(directory);
            ASSERT_TRUE(createdDirectory);
            objPath = directory / "triangle.obj";
            retryPath = directory / "retry.obj";
            WriteTriangle(objPath);
        }
        void TearDown() override
        {
            if (!createdDirectory) return;
            for (const auto& path : { objPath, retryPath, directory })
            {
                std::error_code error;
                std::filesystem::remove(path, error);
                EXPECT_FALSE(error) << error.message();
            }
        }
        void WriteTriangle(const std::filesystem::path& path)
        {
            std::ofstream file(path);
            ASSERT_TRUE(file.is_open());
            file << "v 0 0 0\nv 1 0 0\nv 0 1 0\n"
                 << "vt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\n"
                 << "f 1/1/1 2/2/1 3/3/1\n";
            file.close();
            ASSERT_TRUE(file.good());
        }

        // Renderer must outlive both the cache and any external resource references.
        ResourceProbeRenderer renderer;
        Engine::ResourceManager resources{renderer};
        std::filesystem::path directory, objPath, retryPath;
        bool createdDirectory = false;
        const std::wstring texturePath = L"fake-texture.png";
    };
}

TEST_F(ResourceManagerTest, ReusesMeshForExactSamePath)
{
    auto first = resources.LoadMesh(objPath.string());
    ASSERT_NE(first, nullptr);
    auto second = resources.LoadMesh(objPath.string());
    EXPECT_EQ(first, second);
    EXPECT_EQ(first->GetIndexCount(), 3u);
    EXPECT_EQ(renderer.vertexCalls, 1);
    EXPECT_EQ(renderer.indexCalls, 1);
}

TEST_F(ResourceManagerTest, ReusesTextureForExactSamePath)
{
    auto first = resources.LoadTexture(texturePath);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(resources.LoadTexture(texturePath), first);
    EXPECT_EQ(renderer.textureCalls, 1);
}

TEST_F(ResourceManagerTest, MissingMeshFileIsNotCachedAndCanBeRetried)
{
    EXPECT_EQ(resources.LoadMesh(retryPath.string()), nullptr);
    EXPECT_EQ(renderer.vertexCalls, 0);
    EXPECT_EQ(renderer.indexCalls, 0);
    WriteTriangle(retryPath);
    auto mesh = resources.LoadMesh(retryPath.string());
    ASSERT_NE(mesh, nullptr);
    EXPECT_EQ(resources.LoadMesh(retryPath.string()), mesh);
    EXPECT_EQ(renderer.vertexCalls, 1);
    EXPECT_EQ(renderer.indexCalls, 1);
}

TEST_F(ResourceManagerTest, VertexFailureSkipsIndexCreationAndDoesNotPoisonCache)
{
    renderer.failVertex = true;
    EXPECT_EQ(resources.LoadMesh(objPath.string()), nullptr);
    EXPECT_EQ(renderer.vertexCalls, 1);
    EXPECT_EQ(renderer.indexCalls, 0);
    EXPECT_EQ(renderer.bufferReleases, 0);
    EXPECT_TRUE(renderer.liveBuffers.empty());

    renderer.failVertex = false;
    auto mesh = resources.LoadMesh(objPath.string());
    ASSERT_NE(mesh, nullptr);
    EXPECT_EQ(resources.LoadMesh(objPath.string()), mesh);
    EXPECT_EQ(renderer.vertexCalls, 2);
    EXPECT_EQ(renderer.indexCalls, 1);
    mesh.reset();
    resources.Clear();
    EXPECT_EQ(renderer.bufferReleases, 2);
}

TEST_F(ResourceManagerTest, IndexFailureReleasesVertexOnceAndDoesNotPoisonCache)
{
    renderer.failIndex = true;
    EXPECT_EQ(resources.LoadMesh(objPath.string()), nullptr);
    EXPECT_EQ(renderer.vertexCalls, 1);
    EXPECT_EQ(renderer.indexCalls, 1);
    EXPECT_EQ(renderer.bufferReleases, 1);
    EXPECT_TRUE(renderer.liveBuffers.empty());

    renderer.failIndex = false;
    auto mesh = resources.LoadMesh(objPath.string());
    ASSERT_NE(mesh, nullptr);
    EXPECT_EQ(resources.LoadMesh(objPath.string()), mesh);
    EXPECT_EQ(renderer.vertexCalls, 2);
    EXPECT_EQ(renderer.indexCalls, 2);
    mesh.reset();
    resources.Clear();
    EXPECT_EQ(renderer.bufferReleases, 3);
    resources.Clear();
    EXPECT_EQ(renderer.bufferReleases, 3);
}

TEST_F(ResourceManagerTest, TextureFailureIsNotCachedAndCanBeRetried)
{
    renderer.failTexture = true;
    EXPECT_EQ(resources.LoadTexture(texturePath), nullptr);
    EXPECT_EQ(renderer.textureCalls, 1);
    EXPECT_EQ(renderer.textureReleases, 0);
    renderer.failTexture = false;
    auto texture = resources.LoadTexture(texturePath);
    ASSERT_NE(texture, nullptr);
    EXPECT_EQ(resources.LoadTexture(texturePath), texture);
    EXPECT_EQ(renderer.textureCalls, 2);
    texture.reset();
    resources.Clear();
    EXPECT_EQ(renderer.textureReleases, 1);
}

TEST_F(ResourceManagerTest, CacheKeepsResourcesAliveUntilClearAndRepeatedClearIsSafe)
{
    auto mesh = resources.LoadMesh(objPath.string());
    auto texture = resources.LoadTexture(texturePath);
    ASSERT_NE(mesh, nullptr);
    ASSERT_NE(texture, nullptr);
    std::weak_ptr<Engine::Mesh> weakMesh = mesh;
    std::weak_ptr<Engine::Texture> weakTexture = texture;
    mesh.reset();
    texture.reset();
    EXPECT_FALSE(weakMesh.expired());
    EXPECT_FALSE(weakTexture.expired());
    EXPECT_EQ(renderer.bufferReleases, 0);
    EXPECT_EQ(renderer.textureReleases, 0);
    resources.Clear();
    EXPECT_TRUE(weakMesh.expired());
    EXPECT_TRUE(weakTexture.expired());
    EXPECT_EQ(renderer.bufferReleases, 2);
    EXPECT_EQ(renderer.textureReleases, 1);
    resources.Clear();
    EXPECT_EQ(renderer.bufferReleases, 2);
    EXPECT_EQ(renderer.textureReleases, 1);
}

TEST_F(ResourceManagerTest, ClearPreservesExternalReferencesUntilLastOwnerReleases)
{
    auto mesh = resources.LoadMesh(objPath.string());
    auto texture = resources.LoadTexture(texturePath);
    ASSERT_NE(mesh, nullptr);
    ASSERT_NE(texture, nullptr);
    auto anotherMesh = mesh;
    auto anotherTexture = texture;
    resources.Clear();
    EXPECT_EQ(renderer.bufferReleases, 0);
    EXPECT_EQ(renderer.textureReleases, 0);
    EXPECT_EQ(renderer.liveBuffers.count(mesh->GetVertexBuffer()), 1u);
    EXPECT_EQ(renderer.liveTextures.count(texture->GetTextureHandle()), 1u);
    mesh.reset();
    texture.reset();
    EXPECT_EQ(renderer.bufferReleases, 0);
    EXPECT_EQ(renderer.textureReleases, 0);
    anotherMesh.reset();
    anotherTexture.reset();
    EXPECT_EQ(renderer.bufferReleases, 2);
    EXPECT_EQ(renderer.textureReleases, 1);
}

TEST_F(ResourceManagerTest, ReloadAfterClearCreatesNewResourcesWithoutInvalidatingOldOnes)
{
    auto oldMesh = resources.LoadMesh(objPath.string());
    auto oldTexture = resources.LoadTexture(texturePath);
    ASSERT_NE(oldMesh, nullptr);
    ASSERT_NE(oldTexture, nullptr);
    resources.Clear();
    auto newMesh = resources.LoadMesh(objPath.string());
    auto newTexture = resources.LoadTexture(texturePath);
    ASSERT_NE(newMesh, nullptr);
    ASSERT_NE(newTexture, nullptr);
    EXPECT_NE(oldMesh, newMesh);
    EXPECT_NE(oldTexture, newTexture);
    EXPECT_EQ(renderer.vertexCalls, 2);
    EXPECT_EQ(renderer.indexCalls, 2);
    EXPECT_EQ(renderer.textureCalls, 2);
    oldMesh.reset();
    oldTexture.reset();
    EXPECT_EQ(renderer.liveBuffers.count(newMesh->GetVertexBuffer()), 1u);
    EXPECT_EQ(renderer.liveBuffers.count(newMesh->GetIndexBuffer()), 1u);
    EXPECT_EQ(renderer.liveTextures.count(newTexture->GetTextureHandle()), 1u);
}

TEST_F(ResourceManagerTest, ManagerDestructionReleasesCacheBeforeRenderer)
{
    {
        Engine::ResourceManager localResources(renderer);
        ASSERT_NE(localResources.LoadMesh(objPath.string()), nullptr);
        ASSERT_NE(localResources.LoadTexture(texturePath), nullptr);
        EXPECT_EQ(renderer.liveBuffers.size(), 2u);
        EXPECT_EQ(renderer.liveTextures.size(), 1u);
    }
    EXPECT_EQ(renderer.bufferReleases, 2);
    EXPECT_EQ(renderer.textureReleases, 1);
    EXPECT_TRUE(renderer.liveBuffers.empty());
    EXPECT_TRUE(renderer.liveTextures.empty());
}

TEST_F(ResourceManagerTest, ManagerDestructionPreservesExternalReferencesWhileRendererLives)
{
    std::shared_ptr<Engine::Mesh> mesh;
    std::shared_ptr<Engine::Texture> texture;
    {
        Engine::ResourceManager localResources(renderer);
        mesh = localResources.LoadMesh(objPath.string());
        texture = localResources.LoadTexture(texturePath);
        ASSERT_NE(mesh, nullptr);
        ASSERT_NE(texture, nullptr);
    }
    EXPECT_EQ(renderer.bufferReleases, 0);
    EXPECT_EQ(renderer.textureReleases, 0);
    mesh.reset();
    texture.reset();
    EXPECT_EQ(renderer.bufferReleases, 2);
    EXPECT_EQ(renderer.textureReleases, 1);
}

TEST_F(ResourceManagerTest, ComponentReferencesSurviveCacheClearAndReleaseOnLevelEnd)
{
    Engine::Level level;
    level.AttachServices(renderer, resources);
    auto actor = std::make_unique<Engine::Actor>();
    auto* component = actor->AddComponent<Engine::MeshRendererComponent>();
    auto mesh = resources.LoadMesh(objPath.string());
    auto texture = resources.LoadTexture(texturePath);
    ASSERT_NE(mesh, nullptr);
    ASSERT_NE(texture, nullptr);
    component->SetMesh(mesh);
    auto material = std::make_shared<Engine::Material>();
    material->SetMainTexture(texture);
    component->SetMaterial(0, material);
    level.AddNewActor(std::move(actor));
    level.ProcessAddAndDestroyActor();
    mesh.reset();
    texture.reset();
    material.reset();
    resources.Clear();
    EXPECT_EQ(renderer.bufferReleases, 0);
    EXPECT_EQ(renderer.textureReleases, 0);

    level.GetRenderingSystem().Render(Engine::Matrix4::identity, Engine::Matrix4::identity, nullptr);
    EXPECT_EQ(renderer.submissions, 1);
    level.EndLevel();
    EXPECT_EQ(renderer.bufferReleases, 2);
    EXPECT_EQ(renderer.textureReleases, 1);
    resources.Clear();
    EXPECT_EQ(renderer.bufferReleases, 2);
    EXPECT_EQ(renderer.textureReleases, 1);
}
