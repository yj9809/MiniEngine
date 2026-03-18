#pragma once
#pragma warning(disable: 4251)	// STL 멤버를 DLL 인터페이스로 노출할 때 발생하는 경고 억제.

// DLL 경계에서 심볼을 내보내거나 가져오기 위한 매크로.
// ENGINE_BUILD_DLL이 정의된 경우(엔진 자체 빌드 시) export, 그 외(사용 측)에는 import.
#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)

#if defined(ENGINE_BUILD_DLL)
#define ENGINE_API DLLEXPORT
#else
#define ENGINE_API DLLIMPORT
#endif
