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
		Input& input = Input::Get();

		for (auto& [key, command] : keyDownBinding)
		{
			if (input.GetKeyDown(key))
			{
				command->Execute();
			}
		}

		for (auto& [key, command] : keyUpBinding)
		{
			if (input.GetKeyUp(key))
			{
				command->Execute();
			}
		}

		for (auto& [key, command] : keyBinding)
		{
			if (input.GetKey(key))
			{
				command->Execute();
			}
		}
	}
}
