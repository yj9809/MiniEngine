#pragma once
#pragma warning(disable: 4251)

#define DLLEXPORT __declspec(dllexport)
#define DLLIMPORT __declspec(dllimport)

#if defined(ENGINE_BUILD_DLL)
#define ENGINE_API DLLEXPORT
#else
#define ENGINE_API DLLIMPORT
#endif
