#pragma once

#include <memory>
#include <vector>
#include <stdexcept>

#include "Common/RTTI.h"
#include "Math/Vector3.h"
#include "Component/Component.h"
#include "Common/LifecycleState.h"

namespace Engine
{
    class Level;
    class TransformComponent;

    // 게임 월드에 존재하는 모든 오브젝트의 베이스 클래스.
    // 언리얼 엔진의 AActor와 유사하게, 생명주기(BeginPlay → Tick → OnDestroy)를
    // 가상 함수로 제공해 파생 클래스에서 게임 로직을 구현한다.
    class ENGINE_API Actor : public RTTI
    {
        RTTI_DECLARATIONS(Actor, RTTI)

    public:
        // 파생 클래스는 기본적으로 생성자에서 Component를 생성하고 AddComponent()로 부착하도록 설계.
        Actor();
        virtual ~Actor();

        Actor(const Actor&) = delete;
        Actor& operator=(const Actor&) = delete;

        // 엔진 내부에서 초기화를 보장하기 위한 DispatchInitialize() 호출. 외부에서 직접 호출하지 말 것.
        void DispatchInitialize();
        // 액터가 레벨에 등록될 때 한 번 호출된다. 초기화 로직을 여기에 구현한다.
        virtual void Initialize();

        // 엔진 내부에서 BeginPlay를 보장하기 위한 DispatchBeginPlay() 호출. 외부에서 직접 호출하지 말 것.
        void DispatchBeginPlay();
        // 액터가 초기화까지 끝나면 BeginPlay()가 호출된다. Tick()이 호출되기 전에 한 번만 호출된다.
        virtual void BeginPlay();

        // 매 프레임 호출된다. deltaTime은 초 단위 프레임 경과 시간.
        virtual void Tick(float deltaTime);

        // 엔진 내부에서 제거를 보장하기 위한 DispatchOnDestroy() 호출. 외부에서 직접 호출하지 말 것.
        void DispatchOnDestroy();

        // 컴포넌트를 생성해 이 액터에 부착하고 raw pointer를 반환한다.
        // 소유권은 components 벡터가 가지며, 반환된 포인터는 관찰 용도로만 사용한다.
        template <typename T>
        T* AddComponent()
        {
            // Actor 초기화 시작 전에만 컴포넌트를 추가할 수 있도록 제한한다.
            // 아후 std::logic_error를 던지며 Actor를 변경하지 않는다.
            if (lifecycleState != LifecycleState::Constructed)
            {
                throw std::logic_error("Components can only be added during the Constructed state.");
            }

            auto newComponent = std::make_unique<T>();
            T* ptr = newComponent.get();

            components.emplace_back(std::move(newComponent));
            ptr->DispatchOnAdd(*this);
            
            return ptr;
        }

        template <typename T>
        T* GetComponent()
        {
            for (auto& com : components)
            {
                if (com->IsTypeOf<T>())
                {
                    return com->As<T>();
                }
            }

            return nullptr;
        }

        // Getter/Setter.
        void SetPosition(const Vector3& position);
        /**
         * 루트 Transform의 로컬 자세를 Euler 각도로 설정합니다.
         * rotation.x=Pitch, y=Yaw, z=Roll이며 단위는 degree입니다.
         * 기존 자세에 더하지 않고 새 자세를 지정하며, 내부에는 Quaternion으로 저장합니다.
         * 변환 또는 정규화 실패 시 기존 자세를 유지하고 변경 알림을 호출하지 않습니다.
         */
        void SetRotation(const Vector3& rotation);
        void SetScale(const Vector3& scale);
        
        // 현재 포지션 반환 (로컬).
        Vector3 GetPosition() const;
        /**
         * 루트 Transform의 현재 Quaternion 자세를 Euler 각도로 표현해 반환합니다.
         * 반환값은 x=Pitch, y=Yaw, z=Roll이며 단위는 degree입니다.
         * Pitch는 [-90, 90], Yaw와 Roll은 [-180, 180) 범위입니다.
         * 극점 근처에서는 Roll=0인 대표 표현을 반환합니다.
         * 마지막 입력 각도, 누적 회전 횟수, 각도의 연속성을 보장하지 않습니다.
         * 조회는 자세를 변경하지 않으며, 내부 변환 실패 시 zero를 반환합니다.
         */
        Vector3 GetRotation() const;
        Vector3 GetScale() const;

        inline void SetOwner(Level* newOwner) { owner = newOwner; }
        inline Level* GetOwner() const { return owner; }

        inline TransformComponent* GetRootComponent() const { return rootComponent; }
        
        inline bool IsActive() const { return isActive; }
        bool IsPendingDestroy() const;

    protected:
        // 사용자 정의 삭제 로직을 구현할 수 있는 가상 함수. 액터 제거 요청 시 호출된다.
        virtual void OnDestroy();

    protected:
        // false가 되면 Tick/Draw 대상에서 제외된다.
        // DispatchOnDestroy()가 종료 처리 시작 시 설정한다.
        bool isActive = true;

        // 이 액터를 소유한 Level. 소유권은 Level에 있으므로 raw pointer 사용.
        Level* owner = nullptr;
        
        // Transform을 root로 고정.
        // Actor는 기본적으로 Transform을 소유하도록 설계.
        TransformComponent* rootComponent = nullptr;

    private:
        // 이 액터에 부착된 컴포넌트 목록. Actor가 unique_ptr로 소유권 관리.
        std::vector<std::unique_ptr<Component>> components;
        
        // 액터의 생명주기 상태.
        // Constructed -> Initializing -> Initialized
        // -> BeginningPlay -> HasBegunPlay
        // -> EndingPlay -> HasEndedPlay
        LifecycleState lifecycleState = LifecycleState::Constructed;
    };
}
