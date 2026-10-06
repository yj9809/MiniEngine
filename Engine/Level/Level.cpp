#include "Level.h"

#include <cassert>

#include "Actor/Actor.h"
#include "Renderer/IRenderer.h"
#include "Resource/ResourceManager.h"	
#include "System/LightingSystem.h"
#include "System/RenderingSystem.h"

namespace Engine
{
	Level::Level()
	{
	}
	
	Level::~Level()
	{
	}
	
	void Level::BeginPlay()
	{
	}

	void Level::Tick(float deltaTime)
	{
		for (auto& actor : actors)
		{
			if (!actor->IsActive())
				continue;

			actor->Tick(deltaTime);
		}
	}
	
	void Level::Draw()
	{
		for (auto& actor : actors)
		{
			if (actor->IsActive())
			{
				actor->Draw();
			}
		}
	}

	void Level::AddNewActor(std::unique_ptr<Actor> actor)
	{
		actor->SetOwner(this);
		actorsToAdd.emplace_back(std::move(actor));
	}

	void Level::ProcessAddAndDestroyActor()
	{
		// 생명주기가 HasEndedPlay인 액터 제거.
		// swap + pop_back 패턴: erase()는 뒤 원소를 전부 앞으로 당기는 O(n) 복사가 발생하지만,
		// 마지막 원소와 교환 후 pop_back하면 O(1)로 제거할 수 있다. (순서 보장 불필요)
		for (std::size_t i = 0; i < actors.size();)
		{
			if (actors[i]->IsPendingDestroy())
			{
				std::swap(actors[i], actors.back());
				actors.pop_back();
			}
			else
			{
				i++;
			}
		}

		// 대기 버퍼(actorsToAdd)의 액터를 실제 목록으로 이동.
		// 대기 중 이미 종료된 Actor는 Initialize/BeginPlay하지 않는다.	
		for (auto& actor : actorsToAdd)
		{
			if (actor->IsPendingDestroy())
			{
				continue;
			}

			// 생명주기 순서를 고려해 DispatchInitialize() → DispatchBeginPlay() 순으로 호출.
			actor->DispatchInitialize();
			actor->DispatchBeginPlay();
			
			actors.emplace_back(std::move(actor));
		}

		actorsToAdd.clear();
	}
	
	void Level::EndLevel()
	{
		for (auto& actor : actors)
		{
			actor->DispatchOnDestroy();
		}
		
		for (auto& actor : actorsToAdd)
		{
			actor->DispatchOnDestroy();
		}

		if (renderingSystem)
		{
			renderingSystem->Clear();
		}
		if (lightingSystem)
		{
			lightingSystem->Clear();
		}
		
		mainCamera = nullptr;
		
		actors.clear();
		actorsToAdd.clear();
	}

	LightingSystem* Level::GetLightingSystem()
	{
		if(!lightingSystem)
		{
			lightingSystem = std::make_unique<LightingSystem>();
		}

		return lightingSystem.get();
	}

	void Level::AttachServices(IRenderer& renderer, ResourceManager& resourceManager)
	{
		this->resourceManager = &resourceManager;
		renderingSystem = std::make_unique<RenderingSystem>(renderer);
	}

	ResourceManager& Level::GetResourceManager() const
	{
		assert(resourceManager != nullptr);
		return *resourceManager;
	}

	RenderingSystem& Level::GetRenderingSystem() const
	{
		assert(renderingSystem != nullptr);
		return *renderingSystem;
	}
}
