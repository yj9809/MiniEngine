#include <cstdint>

namespace Engine
{
    // Render Pass 타입 지정용 열거형.
    enum class RenderLayerType : uint8_t
    {
        Opaque, // 기본 메쉬.
        Transparent, // 투명 메쉬.
        Wireframe, // 와이어프레임. 
        UI // UI 요소. 
    };
}
