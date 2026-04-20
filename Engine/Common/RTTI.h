#pragma once

#include "Common/Common.h"

namespace Engine
{
    // 런타임 타입 식별(RTTI) 베이스 클래스.
    // 언리얼 엔진의 UObject RTTI와 유사하게, C++ 표준 dynamic_cast 대신
    // 직접 타입 ID를 비교해 성능과 제어권을 확보한다.
    // 파생 클래스에서 RTTI_DECLARATIONS 매크로를 선언해 사용한다.
    class ENGINE_API RTTI
    {
    public:
        // 클래스 고유 타입 ID를 반환한다. 파생 클래스에서 override 필수.
        virtual size_t GetType() const = 0;

        // 포인터 기반 타입 비교. 기본 구현은 false를 반환하며 파생 클래스에서 override된다.
        virtual bool Is(RTTI* const rtti) const
        {
            return false;
        }

        // ID 기반 타입 비교. 기본 구현은 false를 반환하며 파생 클래스에서 override된다.
        virtual bool Is(const size_t id) const
        {
            return false;
        }

        // 이 객체가 T 타입(또는 T의 파생 타입)인지 확인한다.
        template <typename T>
        bool IsTypeOf()
        {
            return Is(T::TypeIdClass());
        }

        // 타입이 일치하면 T*로 캐스팅해 반환하고, 아니면 nullptr을 반환한다.
        template <typename T>
        T* As()
        {
            if (Is(T::TypeIdClass()))
            {
                return (T*)this;
            }

            return nullptr;
        }

        // const 버전 As().
        template <typename T>
        T* As() const
        {
            if (Is(T::TypeIdClass()))
            {
                return (T*)this;
            }

            return nullptr;
        }
    };
}

// RTTI 지원을 파생 클래스에 주입하는 매크로.
// TypeIdClass(): 정적 지역 변수의 주소를 타입 ID로 사용한다.
//   → 프로그램 생애 동안 단 하나의 주소가 보장되므로 타입별로 고유하다.
// Is(): 자신의 ID와 일치하지 않으면 부모 클래스의 Is()를 재귀 호출해 상속 계층 전체를 검사한다.
// super: 부모 타입을 using으로 노출해 파생 클래스에서 super::함수() 호출을 가능하게 한다.
#define RTTI_DECLARATIONS(Type, ParentType)												\
friend class RTTI;																		\
protected:																				\
    static const size_t TypeIdClass()													\
    {																					\
        static int runTimeTypeId = 0;													\
        return reinterpret_cast<size_t>(&runTimeTypeId);								\
    }																					\
public:																					\
    virtual size_t GetType() const override { return Type::TypeIdClass(); }		\
    using super = ParentType;															\
    virtual bool Is(const size_t id) const override										\
    {																					\
        if (id == TypeIdClass())														\
        {																				\
            return true;																\
        }																				\
        else																			\
        {																				\
            return ParentType::Is(id);													\
        }																				\
    }																					\
    virtual bool Is(RTTI* const rtti) const override									\
    {																					\
        return Is(rtti->GetType());														\
    }
