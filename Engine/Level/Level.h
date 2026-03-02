#pragma once

#include "Common/RTTI.h"

#include <vector>
#include <memory>

namespace Engine
{
	class Actor;

	class ENGINE_API Level : public RTTI
	{
		RTTI_DECLARATIONS(Level, RTTI)

	public:
		Level();
		virtual ~Level();

		virtual void Start();
		virtual void Update(float deltaTime);
		virtual void Draw();

		void AddNewActor(std::unique_ptr<Actor> actor);

		void ProcessAddAndDestroyActor();

	protected:
		std::vector<std::unique_ptr<Actor>> actors;

		std::vector<std::unique_ptr<Actor>> actorsToAdd;
	};
}
