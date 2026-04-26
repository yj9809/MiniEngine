#include "Log.h"

namespace Engine
{
    void Log::Output(std::string_view level, std::string_view msg)
    {
#ifdef _DEBUG
        std::string output = std::string(level) + " " + std::string(msg) + "\n";
        OutputDebugStringA(output.c_str());
        if (level == "[ERROR]")
        {
            std::cerr << output;
        }
        else
        {
            std::cout << output;
        }
#endif
    }
}
