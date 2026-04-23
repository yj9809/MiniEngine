#pragma once

#include "Common/Common.h"
#include "Common/RTTI.h"
#include "Component/Component.h"

namespace Engine
{
    class ENGINE_API TransformComponent : public Component
    {
        RTTI_DECLARATIONS(TransformComponent, Component)
        
    public:
        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = delete;
        TransformComponent& operator=(const TransformComponent&) = delete;
        
    };
}

