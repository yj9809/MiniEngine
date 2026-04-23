#pragma once

#include "Common/Common.h"
#include "RenderLayer.h"

#include <unordered_map>
#include <memory>

namespace Engine
{
    // Render Pass의 드로우 스케줄 담당 클래스.
    class ENGINE_API LayerScheduler
    {
    public:
        LayerScheduler() = default;
        // 복사 생성자/대입 연산자 삭제.
        LayerScheduler(const LayerScheduler&) = delete;
        LayerScheduler& operator=(const LayerScheduler&) = delete;
        
        // Pass Scheduler 등록 함수.
        void RegisterPassScheduler(RenderLayerType type, std::unique_ptr<RenderLayer> renderPass);
        
        // 해당 Pass Type에 RenderPass 반환 함수.
        RenderLayer* GetPass(RenderLayerType type) const;
    private:
        // 각 패스마다 RenderPass를 저장하는 맵.
        std::unordered_map<RenderLayerType, std::unique_ptr<RenderLayer>> passMap;
    };
}
