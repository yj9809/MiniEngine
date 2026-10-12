#pragma once

#include <exception>
#include <utility>

namespace Engine
{
    // 각 정리 작업을 시도하고, 처음 발생한 예외를 보관한다.
    class CleanupExceptionCollector final
    {
    public:
        template <typename Operation>
        void Try(Operation&& operation) noexcept
        {
            try
            {
                std::forward<Operation>(operation)();
            }
            catch (...)
            {
                if (!firstError)
                {
                    firstError = std::current_exception();
                }
            }
        }

        // 모든 정리 작업을 시도한 뒤 명시적으로 호출한다.
        void RethrowIfAny() const
        {
            if (firstError)
            {
                std::rethrow_exception(firstError);
            }
        }

    private:
        std::exception_ptr firstError;
    };
}
