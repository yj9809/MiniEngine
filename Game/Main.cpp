#include <cstdlib>
#include <iostream>
#include <utility>

#include "Actor/CameraActor.h"
#include "Actor/TestMeshActor.h"
#include "Actor/DirectionalLightActor.h"
#include "Engine/Engine.h"
#include "Level/Level.h"
#include "Core/Input.h"

int main()
{
	auto startupLevel = std::make_unique<Engine::Level>();

	startupLevel->AddNewActor(std::make_unique<CameraActor>());
	startupLevel->AddNewActor(std::make_unique<TestMeshActor>());
	startupLevel->AddNewActor(std::make_unique<DirectionalLightActor>());

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
