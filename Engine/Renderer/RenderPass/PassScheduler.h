#pragma once

#include "Common/Common.h"
#include "RenderPass.h"

#include <unordered_map>
#include <memory>

namespace Engine
{
    // Render Pass의 드로우 스케줄 담당 클래스.
    class ENGINE_API PassScheduler
    {
    public:
        PassScheduler() = default;
        // 복사 생성자/대입 연산자 삭제.
        PassScheduler(const PassScheduler&) = delete;
        PassScheduler& operator=(const PassScheduler&) = delete;
        
        // Pass Scheduler 등록 함수.
        void RegisterPassScheduler(RenderPassType type, std::unique_ptr<RenderPass> renderPass);
        
        // 해당 Pass Type에 RenderPass 반환 함수.
        RenderPass* GetPass(RenderPassType type) const;
    private:
        // 각 패스마다 RenderPass를 저장하는 맵.
        std::unordered_map<RenderPassType, std::unique_ptr<RenderPass>> passMap;
    };
}
