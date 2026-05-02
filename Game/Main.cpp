#include "Actor/QuadActor.h"
#include "Actor/TriangleActor.h"
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
	
	// auto triangle = std::make_unique<TriangleActor>();
	// triangle->Init(engine.GetRenderer());
	// levelPtr->AddNewActor(std::move(triangle));
	//
	// auto quad = std::make_unique<QuadActor>();
	// quad->Init(engine.GetRenderer());
	// levelPtr->AddNewActor(std::move(quad));
	
	auto testMsh = std::make_unique<TestMeshActor>();
	testMsh->Initialize(engine.GetRenderer());
	levelPtr->AddNewActor(std::move(testMsh));
	
	engine.Run();
}
