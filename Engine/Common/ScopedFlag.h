#pragma once

namespace Engine
{
    struct ScopedFlag final
    {
    public:
        ScopedFlag(bool& flag) noexcept
            : flag(flag), previousValue(flag)
        {
            flag = true;
        }

        ~ScopedFlag() noexcept
        {
            flag = previousValue;
        }
        
        ScopedFlag(const ScopedFlag&) = delete;
        ScopedFlag& operator=(const ScopedFlag&) = delete;
        ScopedFlag(ScopedFlag&&) = delete;
        ScopedFlag& operator=(ScopedFlag&&) = delete;

    private:
        bool& flag;
        bool previousValue;
    };
    
}