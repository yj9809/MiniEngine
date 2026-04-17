#include "Level.h"
#include "Actor/Actor.h"

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
		// destroyRequested 액터 제거.
		// swap + pop_back 패턴: erase()는 뒤 원소를 전부 앞으로 당기는 O(n) 복사가 발생하지만,
		// 마지막 원소와 교환 후 pop_back하면 O(1)로 제거할 수 있다. (순서 보장 불필요)
		for (int i = 0; i < actors.size();)
		{
			if (actors[i]->IsDestroyRequested())
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
		// BeginPlay()를 여기서 호출해, AddNewActor() 시점이 아닌 프레임 경계에서 초기화되도록 보장한다.
		for (auto& actor : actorsToAdd)
		{
			actor->BeginPlay();
			actors.emplace_back(std::move(actor));
		}

		actorsToAdd.clear();
	}
	void Level::EndLevel()
	{
		actors.clear();
		actorsToAdd.clear();
	}
}
