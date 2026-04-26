#pragma once

#include <format>
#include <string_view>
#include <iostream>
#include <Windows.h>

namespace Engine
{
    // FAILCHECK(HRESULT 기반)로 처리하지 않는 경우에도 로그를 남기기 위한 간단한 로그 클래스.
    class Log
    {
    public:
        // 로그는 정적 함수만 제공하는 유틸리티 클래스이므로, 인스턴스 생성을 막는다.
        Log() = delete;
        Log(const Log&) = delete;
        Log(Log&&) = delete;
        Log& operator=(const Log&) = delete;
        Log& operator=(Log&&) = delete;
        
    public:
        template <typename... Args>
        static void Error(std::format_string<Args...> format, Args&&... args)
        {
            Output("[ERROR]", std::format(format, std::forward<Args>(args)...));
        }
        template <typename... Args>
        static void Warning(std::format_string<Args...> format, Args&&... args)
        {
            Output("[WARNING]", std::format(format, std::forward<Args>(args)...));
        }
        template <typename... Args>
        static void Info(std::format_string<Args...> format, Args&&... args)
        {
            Output("[INFO]", std::format(format, std::forward<Args>(args)...));
        }
        
    private:
        static void Output(std::string_view level, std::string_view msg);
    };
}
