#include "Actor/QuadActor.h"
#include "Actor/TriangleActor.h"
#include "Engine/Engine.h"
#include "Level/Level.h"
#include "Core/Input.h"

int main()
{
	Engine::Engine engine;

	auto level = std::make_unique<Engine::Level>();
	Engine::Level* levelPtr = level.get();
	engine.SetNewLevel(std::move(level));

	auto triangle = std::make_unique<TriangleActor>();
	triangle->Init(engine.GetRenderer());
	levelPtr->AddNewActor(std::move(triangle));

	
	auto quad = std::make_unique<QuadActor>();
	quad->Init(engine.GetRenderer());
	levelPtr->AddNewActor(std::move(quad));
	
	engine.Run();
}
