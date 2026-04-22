#include "PassScheduler.h"

namespace Engine
{
    void PassScheduler::RegisterPassScheduler(RenderPassType type, std::unique_ptr<RenderPass> renderPass)
    {
        // 해당 PassType에 RenderPass 등록.
        passMap[type] = std::move(renderPass);
    }

    RenderPass* PassScheduler::GetPass(RenderPassType type) const
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
