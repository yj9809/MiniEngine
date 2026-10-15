#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>

#include "Actor/CameraActor.h"
#include "Actor/TestMeshActor.h"
#include "Actor/DirectionalLightActor.h"
#include "Engine/Engine.h"
#include "Level/Level.h"
#include "Core/Input.h"
#include "Component/Light/SkyLightComponent.h"

int main(int argc, char* argv[])
{
	const bool coloredAmbientDemo = argc > 1 && std::string_view(argv[1]) == "--colored-ambient";
	auto startupLevel = std::make_unique<Engine::Level>();

	auto camera = std::make_unique<CameraActor>();
	if (coloredAmbientDemo) camera->SetPosition({ -1.2f, 0.0f, 0.0f });
	startupLevel->AddNewActor(std::move(camera));
	startupLevel->AddNewActor(std::make_unique<TestMeshActor>());
	if (coloredAmbientDemo)
	{
		auto environment = std::make_unique<Engine::Actor>();
		auto* skyLight = environment->AddComponent<Engine::SkyLightComponent>();
		// Linear RGB: cool sky and warm ground, with no directional light.
		skyLight->SetSkyColor({ 0.12f, 0.45f, 1.0f });
		skyLight->SetGroundColor({ 1.0f, 0.2f, 0.06f });
		skyLight->SetIntensity(1.0f);
		startupLevel->AddNewActor(std::move(environment));
	}
	else
	{
		startupLevel->AddNewActor(std::make_unique<DirectionalLightActor>());
	}

	Engine::EngineCreateInfo createInfo;
	createInfo.startupLevel = std::move(startupLevel);

	auto createResult = Engine::Engine::Create(std::move(createInfo));
	
	if (const auto* error = std::get_if<Engine::EngineInitError>(&createResult))
	{
		std::cerr
			<< "Engine initialization failed: "
			<< error->errorMessage
			<< '\n';

		return EXIT_FAILURE;
	}

	auto engine = std::move(std::get<std::unique_ptr<Engine::Engine>>(createResult));
	
	engine->Run();
	return EXIT_SUCCESS;
}
