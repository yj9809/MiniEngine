#pragma once

#include "../Component.h"
#include "../ICommand.h"

#include "Common/Common.h"
#include "Common/RTTI.h"

#include <memory>
#include <unordered_map>

namespace Engine
{
	class ENGINE_API InputComponent : public Component
	{
		RTTI_DECLARATIONS(InputComponent, Component)

	public:
		void BindKeyDown(int key, std::unique_ptr<ICommand> command);
		void BindKeyUp(int key, std::unique_ptr<ICommand> command);
		void BindKey(int key, std::unique_ptr<ICommand> command);

		void Tick(float deltaTime) override;

	private:
		std::unordered_map<int, std::unique_ptr<ICommand>> keyDownBinding;
		std::unordered_map<int, std::unique_ptr<ICommand>> keyUpBinding;
		std::unordered_map<int, std::unique_ptr<ICommand>> keyBinding;
	};
}

