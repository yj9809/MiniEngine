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
	auto createResult = Engine::Engine::Create();
	
	if (const auto* error = std::get_if<Engine::EngineInitError>(&createResult))
	{
		std::cerr
			<< "Engine initialization failed: "
			<< error->errorMessage
			<< '\n';

		return EXIT_FAILURE;
	}

	auto engine = std::move(std::get<std::unique_ptr<Engine::Engine>>(createResult));
	
	auto level = std::make_unique<Engine::Level>();
	Engine::Level* levelPtr = level.get();
	engine->SetNewLevel(std::move(level));

	auto Camera = std::make_unique<CameraActor>();
	levelPtr->AddNewActor(std::move(Camera));
	
	auto testMsh = std::make_unique<TestMeshActor>();
	levelPtr->AddNewActor(std::move(testMsh));

	auto directionalLight = std::make_unique<DirectionalLightActor>();
	levelPtr->AddNewActor(std::move(directionalLight));
	
	engine->Run();
	return EXIT_SUCCESS;
}
