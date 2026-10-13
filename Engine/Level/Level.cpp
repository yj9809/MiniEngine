#include "Level.h"

#include <cassert>
#include <stdexcept>

#include "Actor/Actor.h"
#include "Common/CleanupExceptionCollector.h"
#include "Renderer/IRenderer.h"
#include "Resource/ResourceManager.h"	
#include "System/LightingSystem.h"
#include "System/RenderingSystem.h"
#include "Component/Camera/CameraComponent.h"
#include "Common/ScopedFlag.h"

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
		if (!renderingSystem || !mainCamera)
		{
			return;
		}

		// 메인 카메라의 뷰/투영 행렬을 렌더링 시스템에 전달.
		renderingSystem->Render(mainCamera->GetViewMatrix(), mainCamera->GetProjectionMatrix(), lightingSystem.get());
	}

	void Level::AddNewActor(std::unique_ptr<Actor> actor)
	{
		if (!actor)
		{
			throw std::invalid_argument("Actor must not be null.");
		}

		if (hasEnded)
		{
			throw std::logic_error("Actors cannot be added to an ending or ended Level.");
		}

		if (isProcessingActors)
		{
			throw std::logic_error("Actors cannot be added during actor processing.");
		}

		actor->SetOwner(this);
		actorsToAdd.emplace_back(std::move(actor));
	}

	void Level::ProcessAddAndDestroyActor()
	{
		if (hasEnded)
		{
			return;
		}
		
		if (isProcessingActors)
		{
			throw std::logic_error("Actor processing cannot be reentered.");
		}

		ScopedFlag guard(isProcessingActors);

		// 생명주기가 HasEndedPlay인 액터 제거.
		// swap + pop_back 패턴: erase()는 뒤 원소를 전부 앞으로 당기는 O(n) 복사가 발생하지만,
		// 마지막 원소와 교환 후 pop_back하면 O(1)로 제거할 수 있다. (순서 보장 불필요)
		for (std::size_t i = 0; i < actors.size();)
		{
			if (actors[i]->IsPendingDestroy())
			{
				if (mainCamera && &mainCamera->GetOwner() == actors[i].get())
				{
					mainCamera = nullptr;
				}
				
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
				if (mainCamera && &mainCamera->GetOwner() == actor.get())
				{
					mainCamera = nullptr;
				}

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
		if (hasEnded)
		{
			return;
		}

		if (isProcessingActors)
		{
			throw std::logic_error("Level cannot end during actor processing.");
		}

		// 콜백 안에서 EndLevel()이 재호출돼도 막을 수 있도록 실제 정리보다 먼저 설정한다.
		hasEnded = true;
		CleanupExceptionCollector cleanup;

		for (auto& actor : actors)
		{
			if (actor)
			{
				cleanup.Try([&]()
				{
					actor->DispatchOnDestroy();
				});
			}
		}
		
		for (auto& actor : actorsToAdd)
		{
			if (actor)
			{
				cleanup.Try([&]()
				{
					actor->DispatchOnDestroy();
				});
			}
		}

		if (renderingSystem)
		{
			cleanup.Try([&]()
			{
				renderingSystem->Clear();
			});
		}
		if (lightingSystem)
		{
			cleanup.Try([&]()
			{
				lightingSystem->Clear();
			});
		}
		
		mainCamera = nullptr;
		actors.clear();
		actorsToAdd.clear();
		cleanup.RethrowIfAny();
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
