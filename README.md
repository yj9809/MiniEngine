# MiniEngine

C++ 기반 학습용 미니 게임 엔진.
언리얼 엔진의 설계 철학을 참고하여 구조를 잡고, 엔진 프로그래머 역량 향상을 목표로 한다.

---

## 목표

- 엔진의 핵심 시스템(렌더러, 입력, 충돌, 컴포넌트 등)을 직접 구현하며 내부 동작 원리를 이해한다.
- 언리얼 엔진 학습으로 이어지는 브릿지 프로젝트로 활용한다.

---

## 프로젝트 구조

```
Engine/
├── Actor/          # 게임 오브젝트 베이스 클래스
├── Common/         # 공통 매크로, RTTI
├── Engine/         # 엔진 코어, 게임 루프
├── Level/          # 씬(레벨) 관리
├── Math/           # Vector2 등 수학 유틸리티
├── Renderer/       # 화면 버퍼 (콘솔 기반)
└── Setting/        # 엔진 설정 파일
```

---

## 설계 원칙

| 항목 | 내용 |
|------|------|
| 메모리 관리 | `unique_ptr` 우선 사용. raw pointer는 소유권 없는 참조에만 허용 |
| 네이밍 | 언리얼 스타일: `BeginPlay`, `Tick`, `OnDestroy` |
| 설정 파일 | `std::ifstream` 사용 |
| 싱글턴 | `Input`, `Renderer`에만 제한적으로 허용 |
| 의존성 방향 | 게임 → 엔진 단방향. 엔진 코드는 게임 코드에 의존하지 않는다 |

---

## 진행 현황

### ✅ 1단계 — 코어 정리

엔진의 뼈대와 라이프사이클을 올바르게 정립한 단계.

**네이밍 통일**
- `Start()` → `BeginPlay()`, `update()` → `Tick()` (언리얼 스타일)

**BeginPlay 라이프사이클 재설계**

초기에는 `hasBeginPlay` 플래그를 Actor에 두고 게임 루프에서 매 프레임 체크하는 방식을 검토했으나,
"월드에 진입하는 시점에 1회 호출"이라는 의미를 더 명확히 표현하는 구조로 재설계했다.

- `Actor::BeginPlay()` — `ProcessAddAndDestroyActor()`에서 액터가 `actors` 리스트에 편입될 때 1회 호출
- `Level::BeginPlay()` — `Engine::SetNewLevel()`에서 레벨이 세팅될 때 1회 호출
- 게임 루프는 `Tick()` + `Draw()`만 담당

```cpp
// Actor — 월드 편입 시 BeginPlay 호출
void Level::ProcessAddAndDestroyActor()
{
    for (auto& actor : actorsToAdd)
    {
        actor->BeginPlay();  // 월드에 들어오는 순간 1회
        actors.emplace_back(std::move(actor));
    }
    actorsToAdd.clear();
}

// Level — 레벨 세팅 시 BeginPlay 호출
void Engine::SetNewLevel(std::unique_ptr<Level> level)
{
    mainLevel = std::move(level);
    mainLevel->BeginPlay();  // 레벨이 세팅되는 순간 1회
}
```

---

## 로드맵

| 단계 | 내용 | 상태 |
|------|------|------|
| 1단계 | 코어 정리 (네이밍, 라이프사이클) | ✅ 완료 |
| 2단계 | Renderer (더블 버퍼링, RenderCommand) | 🔲 예정 |
| 3단계 | Input (Pressed / Held / Released) | 🔲 예정 |
| 4단계 | Component 시스템 (`AddComponent<T>()`) | 🔲 예정 |
| 5단계 | 충돌 시스템 (AABB, BVH) | 🔲 예정 |
| 6단계 | Time 시스템 (DeltaTime, TimeScale) | 🔲 예정 |
| 7단계 | 리소스 관리 (캐싱) | 🔲 예정 |
| 8단계 | Level 고도화 (전환, 스택) | 🔲 예정 |
| 9단계 | 메모리 관리 (Object Pool, Custom Allocator) | 🔲 예정 |
