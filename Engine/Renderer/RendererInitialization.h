#pragma once

#include <optional>
#include <string>

namespace Engine
{
    enum class RendererInitialization
    {
        InvalidState,
        Device,
        SwapChain,
        RenderTarget,
        DepthStencil,
        RenderLayer,
        PlatformRuntime
    };

    struct RendererInitError
    {
        RendererInitialization rendererInitialization;
        std::string errorMessage;
    };

    using RendererInitResult = std::optional<RendererInitError>;
}
