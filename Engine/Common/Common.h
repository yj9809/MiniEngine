#pragma once
#pragma warning(disable: 4251)	// STL 멤버를 DLL 인터페이스로 노출할 때 발생하는 경고 억제.

#include <Windows.h>
#include <comdef.h>

// DLL 경계에서 심볼을 내보내거나 가져오기 위한 매크로.
// ENGINE_BUILD_DLL이 정의된 경우(엔진 자체 빌드 시) export, 그 외(사용 측)에는 import.
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)

#if defined(ENGINE_BUILD_DLL)
#define ENGINE_API DLLEXPORT
#else
#define ENGINE_API DLLIMPORT
#endif

// 에러 처리 로직을 별도 함수로 분리.
// 매크로 인라인 전개 시 GPUInit 스택에 버퍼가 쌓이는 문제 해결.
inline void HandleD3DError(HRESULT hr, const wchar_t* message, const wchar_t* file, int line, const wchar_t* func)
{
    _com_error error(hr);
    const wchar_t* reason = error.ErrorMessage();
    wchar_t buffer[2048] = {};
    swprintf_s(buffer, 2048,
        L"[D3D Error]\n"
        L"Message: %s\n"
        L"Reason: %s\n"
        L"File: %s\n"
        L"Line: %d\n"
        L"Function: %s\n",
        message, reason, file, line, func);
    MessageBoxW(nullptr, buffer, L"D3D Error", MB_OK);
    __debugbreak();
}

// FAILCHECK: HRESULT 실패 시 에러 출력 후 false 반환.
#define FAILCHECK(result, message, retval)  \
    if (FAILED(result))                     \
    {                                       \
        HandleD3DError(result, message,     \
            TEXT(__FILE__), __LINE__,       \
            TEXT(__FUNCTION__));            \
        return retval;                      \
    }


constexpr float PI = 3.14159265f;

