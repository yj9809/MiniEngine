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
	}

	void Level::AddNewActor(std::unique_ptr<Actor> actor)
	{
		actor->SetOwner(this);
		actorsToAdd.emplace_back(std::move(actor));
	}

	void Level::ProcessAddAndDestroyActor()
	{
		// Todo: 액터 제거 기능 추가.
		for (int i = 0; i < actors.size();)
		{
			if (actors[i]->IsDestroyRequested())
			{
				actors.erase(actors.begin() + i);
			}
			else
			{
				i++;
			}
		}

		// 새 액터 추가.
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