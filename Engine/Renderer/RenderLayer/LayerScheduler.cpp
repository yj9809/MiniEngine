#include "LayerScheduler.h"

namespace Engine
{
    void LayerScheduler::RegisterPassScheduler(RenderLayerType type, std::unique_ptr<RenderLayer> renderPass)
    {
        // 해당 PassType에 RenderPass 등록.
        passMap[type] = std::move(renderPass);
    }

    RenderLayer* LayerScheduler::GetPass(RenderLayerType type) const
    {
        // Map에서 RenderPass 찾기.
        auto it = passMap.find(type);
        
        if (it != passMap.end())
        {
            // 찾았으면 RenderPass 반환.
            return it->second.get();
        }
        
        // 못 찾으면 nullptr 반환.
        return nullptr;
    }
}
