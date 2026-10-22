#include "TestMeshActor.h"

#include <memory>
#include <cmath>
#include <iostream>

#include "Component/Mesh/MeshRendererComponent.h"
#include "Component/Transform/TransformComponent.h"
#include "Core/Input.h"
#include "Resource/ResourceManager.h"
#include "Level/Level.h"
#include "Renderer/Material.h"
#include "Math/Quaternion.h"

TestMeshActor::TestMeshActor()
{
    meshRenderer = AddComponent<Engine::MeshRendererComponent>();
}

void TestMeshActor::Initialize()
{
    Actor::Initialize();

    Engine::Level* ownerLevel = GetOwner();

    if(!ownerLevel)
    {
        return;
    }

    Engine::ResourceManager& resourceManager = ownerLevel->GetResourceManager();

    meshRenderer->SetMesh(resourceManager.LoadMesh("Asset/world.obj"));

    auto material = std::make_shared<Engine::Material>();
    material->SetMainTexture(resourceManager.LoadTexture(L"Asset/world_giant.jpg"));

    material->SetBaseColor({ 1.0f, 1.0f, 1.0f, 1.0f }); // RGBA
    meshRenderer->SetMaterial(0, material);
}

void TestMeshActor::BeginPlay()
{
    Actor::BeginPlay();

    rootComponent->SetLocalPosition({ 2.0f, 0.0f, 0.0f });

    const Engine::Vector3 initialEulerDeg =
        rootComponent->GetLocalRotationEulerDeg();

    TryResetRotationDemo(initialEulerDeg);
}

void TestMeshActor::Tick(float deltaTime)
{
    Actor::Tick(deltaTime);

    if (Engine::Input::GetKeyDown('4'))
    {
        if (TryResetRotationDemo(Engine::Vector3(90.0f, 0.0f, 0.0f)))
        {
            rotationMode =
                rotationMode == RotationMode::EulerAccumulation
                ? RotationMode::QuaternionIncrement
                : RotationMode::EulerAccumulation;

            std::cout
                << (rotationMode == RotationMode::EulerAccumulation
                    ? "Mode: Euler accumulation"
                    : "Mode: Quaternion increment")
                << " / Reset: Pitch 90\n";
        }
        return;
    }

    if (Engine::Input::GetKeyDown('0'))
    {
        if (TryResetRotationDemo(Engine::Vector3::zero))
            std::cout << "Reset: Identity\n";

        return;
    }

    if (Engine::Input::GetKeyDown('9'))
    {
        if (TryResetRotationDemo(Engine::Vector3(90.0f, 0.0f, 0.0f)))
            std::cout << "Reset: Pitch 90\n";

        return;
    }

    if (Engine::Input::GetKeyDown('1')) rotatePitch = !rotatePitch;
    if (Engine::Input::GetKeyDown('2')) rotateYaw = !rotateYaw;
    if (Engine::Input::GetKeyDown('3')) rotateRoll = !rotateRoll;

    if (!rotatePitch && !rotateYaw && !rotateRoll)
    {
        return;
    }

    if (!std::isfinite(deltaTime) || deltaTime <= 0.0f)
    {
        return;
    }

    const float stepDeg = rotationSpeedDeg * deltaTime;
    if (!std::isfinite(stepDeg))
    {
        return;
    }

    const Engine::Vector3 deltaDeg(
        rotatePitch ? stepDeg : 0.0f,
        rotateYaw   ? stepDeg : 0.0f,
        rotateRoll  ? stepDeg : 0.0f
    );

    const float toRadians = PI / 180.0f;

    if (rotationMode == RotationMode::EulerAccumulation)
    {
        Engine::Vector3 candidateEuler =
            accumulatedEulerDeg + deltaDeg;

        // 같은 자세를 유지하면서 각도 수치가 계속 커지는 것을 방지합니다.
        candidateEuler.x = std::remainder(candidateEuler.x, 360.0f);
        candidateEuler.y = std::remainder(candidateEuler.y, 360.0f);
        candidateEuler.z = std::remainder(candidateEuler.z, 360.0f);

        Engine::Quaternion candidateRotation;
        if (!Engine::Quaternion::TryFromEuler(
            candidateEuler.z * toRadians, // Roll
            candidateEuler.x * toRadians, // Pitch
            candidateEuler.y * toRadians, // Yaw
            candidateRotation))
        {
            return;
        }

        if (rootComponent->TrySetLocalRotation(candidateRotation))
        {
            accumulatedEulerDeg = candidateEuler;
        }
    }
    else
    {
        Engine::Quaternion deltaRotation;
        if (!Engine::Quaternion::TryFromEuler(
            deltaDeg.z * toRadians, // Roll
            deltaDeg.x * toRadians, // Pitch
            deltaDeg.y * toRadians, // Yaw
            deltaRotation))
        {
            return;
        }

        rootComponent->TryRotateLocal(deltaRotation);
    }
}

bool TestMeshActor::TryResetRotationDemo(const Engine::Vector3& initialEulerDeg)
{
    const float toRadians = PI / 180.0f;

    Engine::Quaternion initialRotation;
    if (!Engine::Quaternion::TryFromEuler(
        initialEulerDeg.z * toRadians, // Roll
        initialEulerDeg.x * toRadians, // Pitch
        initialEulerDeg.y * toRadians, // Yaw
        initialRotation))
    {
        return false;
    }

    if (!rootComponent->TrySetLocalRotation(initialRotation))
    {
        return false;
    }

    // 실제 자세 저장에 성공한 뒤 데모 상태도 초기화합니다.
    accumulatedEulerDeg = initialEulerDeg;

    rotatePitch = false;
    rotateYaw = false;
    rotateRoll = false;

    return true;
}
