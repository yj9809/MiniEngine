#include "InputComponent.h"

#include "Core/Input.h"

namespace Engine
{
	void InputComponent::BindKeyDown(int key, std::unique_ptr<ICommand> command)
	{
		keyDownBinding[key] = std::move(command);
	}

	void InputComponent::BindKeyUp(int key, std::unique_ptr<ICommand> command)
	{
		keyUpBinding[key] = std::move(command);
	}

	void InputComponent::BindKey(int key, std::unique_ptr<ICommand> command)
	{
		keyBinding[key] = std::move(command);
	}

	void InputComponent::Tick(float deltaTime)
	{
		for (auto& [key, command] : keyDownBinding)
		{
			if (Input::GetKeyDown(key))
			{
				command->Execute();
			}
		}

		for (auto& [key, command] : keyUpBinding)
		{
			if (Input::GetKeyUp(key))
			{
				command->Execute();
			}
		}

		for (auto& [key, command] : keyBinding)
		{
			if (Input::GetKey(key))
			{
				command->Execute();
			}
		}
	}
}
