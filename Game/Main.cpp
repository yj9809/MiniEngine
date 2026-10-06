#include "Actor/CameraActor.h"
#include "Actor/TestMeshActor.h"
#include "Engine/Engine.h"
#include "Level/Level.h"
#include "Core/Input.h"

int main()
{
	Engine::Engine engine;

	auto level = std::make_unique<Engine::Level>();
	Engine::Level* levelPtr = level.get();
	engine.SetNewLevel(std::move(level));

	auto Camera = std::make_unique<CameraActor>();
	levelPtr->AddNewActor(std::move(Camera));
	
	auto testMsh = std::make_unique<TestMeshActor>();
	levelPtr->AddNewActor(std::move(testMsh));
	
	engine.Run();
}
