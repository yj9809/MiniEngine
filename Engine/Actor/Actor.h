#pragma once

#include "Common/RTTI.h"
#include "Math/Vector3.h"
#include "Component/Component.h"

#include <memory>
#include <vector>

#include "Math/Matrix4.h"

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
        Actor();
        virtual ~Actor();

        Actor(const Actor&) = delete;
        Actor& operator=(const Actor&) = delete;

        // 액터가 레벨에 처음 등록될 때 한 번 호출된다. 초기화 로직을 여기에 구현한다.
        virtual void BeginPlay();

        // 매 프레임 호출된다. deltaTime은 초 단위 프레임 경과 시간.
        virtual void Tick(float deltaTime);

        // 렌더링 단계에서 호출된다.
        virtual void Draw();

        // 액터 제거 요청 시 호출된다.
        // isActive = false, destroyRequested = true를 설정해
        // 다음 ProcessAddAndDestroyActor()에서 실제로 제거되도록 예약한다.
        virtual void OnDestroy();

        // 컴포넌트를 동적으로 생성해 이 액터에 부착하고 raw pointer를 반환한다.
        // 소유권은 components 벡터가 가지며, 반환된 포인터는 관찰 용도로만 사용한다.
        template <typename T>
        T* AddComponent()
        {
            auto newComponent = std::make_unique<T>();
            T* ptr = newComponent.get();
            ptr->owner = this;
            components.emplace_back(std::move(newComponent));
            ptr->OnAdd();
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
        void SetRotation(const Vector3& rotation);
        void SetScale(const Vector3& scale);
        
        // 현재 포지션 반환 (로컬).
        Vector3 GetPosition() const;
        Vector3 GetRotation() const;
        Vector3 GetScale() const;

        inline void SetOwner(Level* newOwner) { owner = newOwner; }
        inline Level* GetOwner() const { return owner; }

        inline TransformComponent* GetRootComponent() const { return rootComponent; }
        
        inline bool IsActive() const { return isActive; }
        inline bool IsDestroyRequested() const { return destroyRequested; }

    protected:
        // false가 되면 Tick/Draw 대상에서 제외된다. OnDestroy()에서 설정.
        bool isActive = true;

        // true가 되면 다음 프레임 처리 후 actors 배열에서 제거된다.
        // 순회 중 즉시 제거하지 않고 플래그로 예약하는 이유:
        // Tick() 도중 배열을 수정하면 이터레이터가 무효화되기 때문이다.
        bool destroyRequested = false;

        // 이 액터를 소유한 Level. 소유권은 Level에 있으므로 raw pointer 사용.
        Level* owner = nullptr;
        
        // Transform을 root로 고정.
        // Actor는 기본적으로 Transform을 소유하도록 설계.
        TransformComponent* rootComponent = nullptr;
        
        // view 행렬.
        // Todo: MeshRenderer가 받기 위해 Getter 구현 해야함.
        Matrix4 viewMatrix = Matrix4::identity;
        Matrix4 projectionMatrix = Matrix4::identity;

    private:
        // 이 액터에 부착된 컴포넌트 목록. Actor가 unique_ptr로 소유권 관리.
        std::vector<std::unique_ptr<Component>> components;
    };
}
