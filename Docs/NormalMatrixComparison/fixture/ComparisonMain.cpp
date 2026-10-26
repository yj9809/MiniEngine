#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>
#include "Actor/CameraActor.h"
#include "Actor/TestMeshActor.h"
#include "Actor/DirectionalLightActor.h"
#include "Engine/Engine.h"
#include "Level/Level.h"
#include "Component/Light/SkyLightComponent.h"

int main(int argc, char* argv[])
{
    const bool uniform = argc > 1 && std::string_view(argv[1]) == "--uniform";
    auto level = std::make_unique<Engine::Level>();
    auto camera = std::make_unique<CameraActor>();
    camera->SetPosition({-1.8f, 0, 0});
    level->AddNewActor(std::move(camera));
    auto mesh = std::make_unique<TestMeshActor>();
    mesh->SetScale(uniform ? Engine::Vector3(1,1,1) : Engine::Vector3(2,1,0.5f));
    level->AddNewActor(std::move(mesh));
    auto light = std::make_unique<DirectionalLightActor>();
    auto* lightPtr = light.get();
    level->AddNewActor(std::move(light));
    auto sky = std::make_unique<Engine::Actor>();
    auto* ambient = sky->AddComponent<Engine::SkyLightComponent>();
    ambient->SetSkyColor({1,1,1});
    ambient->SetGroundColor({1,1,1});
    ambient->SetIntensity(0.06f);
    level->AddNewActor(std::move(sky));
    Engine::EngineCreateInfo info;
    info.startupLevel = std::move(level);
    auto result = Engine::Engine::Create(std::move(info));
    if (const auto* error = std::get_if<Engine::EngineInitError>(&result))
    {
        std::cerr << error->errorMessage << '\n';
        return EXIT_FAILURE;
    }
    auto engine = std::move(std::get<std::unique_ptr<Engine::Engine>>(result));
    // Apply after DirectionalLightActor::Initialize's defaults.
    lightPtr->GetComponent<Engine::DirectionalLightComponent>()->SetIntensity(0.9f);
    lightPtr->SetRotation({-25,-30,0});
    engine->Run();
}
