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
├── Component/      # 컴포넌트 베이스 클래스
├── Core/           # Input 시스템
├── Engine/         # 엔진 코어, 게임 루프
├── Level/          # 씬(레벨) 관리
├── Math/           # Vector2 등 수학 유틸리티
├── Renderer/       # 화면 버퍼 (콘솔 기반)
└── Setting/        # 엔진 설정 파일
Tests/
├── Main.cpp        # Google Test 진입점
├── Vector2Test.cpp # Vector2 단위 테스트
└── LevelTest.cpp   # Level 단위 테스트 (기능 + 성능)
```

---

## 설계 원칙

| 항목 | 내용 |
|------|------|
| 메모리 관리 | `unique_ptr` 우선 사용. raw pointer는 소유권 없는 참조에만 허용. `new`/`delete` 직접 사용 금지 |
| 설정 파일 | `std::ifstream` 사용 (C 스타일 `fopen_s` 금지) |
| 싱글턴 | `Input`, `Renderer`에만 제한적으로 허용 |
| 의존성 방향 | 게임 → 엔진 단방향. 엔진 코드는 게임 코드에 의존하지 않는다 |

### 네이밍 컨벤션 (언리얼 스타일)

| 항목 | 규칙 | 예시 |
|------|------|------|
| 클래스/구조체 | PascalCase | `Actor`, `ScreenBuffer` |
| 함수 | PascalCase | `BeginPlay()`, `Tick()` |
| 멤버 변수 | camelCase | `isActive`, `mainLevel` |
| 생명주기 함수 | 언리얼 명칭 사용 | `BeginPlay`, `Tick`, `OnDestroy`, `OnCollision` |

---

## 단위 테스트

[Google Test](https://github.com/google/googletest) 기반. `Tests` 프로젝트를 시작 프로젝트로 설정 후 실행.

```
[==========] Running 22 tests from 3 test suites.
[  PASSED  ] 22 tests.
```

| 테스트 파일 | 테스트 수 | 검증 항목 |
|---|---|---|
| `Vector2Test.cpp` | 2 | 덧셈, 동등 비교 |
| `LevelTest.cpp` | 12 | Actor 추가/제거/owner/EndLevel/성능 |

---

## 진행 현황

### ✅ 1단계 — 코어 정리

엔진의 뼈대와 라이프사이클을 올바르게 정립한 단계.

**1-1. 네이밍 통일** ✅
- `Start()` → `BeginPlay()`, `update()` → `Tick()` (언리얼 스타일)

**1-2. BeginPlay 라이프사이클 재설계** ✅

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

**1-3. Actor 상태 분리** ✅

기존 `isDestroyed` 단일 플래그를 `isActive` + `destroyRequested` 두 플래그로 분리했다.
"비활성화 상태"와 "삭제 예약 상태"의 의미를 명확히 구분하기 위함이다.

- `isActive` — 현재 Tick/Draw에 참여하는지 여부
- `destroyRequested` — 이번 프레임 말에 제거 예약 여부
- `OnDestroy()` — `virtual` 콜백으로 추가, 파생 클래스에서 오버라이드 가능

```cpp
void Actor::OnDestroy()
{
    isActive = false;
    destroyRequested = true;
}
```

**1-4. 엔진 종료 구조 개선** ✅

Actor에서 엔진 종료를 요청하는 구조를 설계하는 과정에서, `Actor::QuitGame()` 래퍼를 두면
Actor(엔진 레이어)가 Engine을 역참조하는 의존성이 생기는 문제가 있었다.
`QuitEngine()`을 `static`으로 전환해 인스턴스 없이 호출 가능하게 하고,
래퍼 없이 게임 코드에서 직접 호출하는 방식으로 의존성 방향(게임 → 엔진)을 지켰다.

```cpp
// Engine.h
inline static bool isQuit = false;  // 선언 + 정의 통합 (C++17)
static void QuitEngine();

// 게임 코드에서 직접 호출
Engine::Engine::QuitEngine();
```

**1-5. Actor 삭제 성능 개선** ✅

`Level::ProcessAddAndDestroyActor()`의 `vector::erase()` 반복 호출 O(n²) 문제를 swap-and-pop으로 교체.

```cpp
// 수정 전: erase — 삭제마다 뒤 요소 전체 이동 O(n²)
// 수정 후: swap-and-pop — O(n)
for (int i = 0; i < (int)actors.size();)
{
    if (actors[i]->IsDestroyRequested())
    {
        std::swap(actors[i], actors.back());
        actors.pop_back();
    }
    else { i++; }
}
```

| 조건 | 수정 전 | 수정 후 |
|---|---|---|
| 10,000개 전체 삭제 | 1046ms | 12ms |

---

### ✅ 2단계 — Input 시스템

`GetKeyDown` / `GetKey` / `GetKeyUp` 세 가지 키 상태를 `KeyState` 구조체(current + previous 프레임 상태)로 구분.

- `GetKeyDown(key)` — 이번 프레임에 처음 눌린 경우 (`Pressed`)
- `GetKey(key)` — 누르고 있는 동안 (`Held`)
- `GetKeyUp(key)` — 이번 프레임에 뗀 경우 (`Released`)

```cpp
struct KeyState
{
    bool current  = false;  // 현재 프레임
    bool previous = false;  // 이전 프레임
};

// 매 프레임 이전 상태를 저장한 뒤 현재 상태 갱신
bool Input::GetKeyDown(int key) const { return  state.current && !state.previous; }
bool Input::GetKey(int key)     const { return  state.current; }
bool Input::GetKeyUp(int key)   const { return !state.current &&  state.previous; }
```

---

### 🔄 3단계 — Component 시스템

컴포넌트를 Actor에 붙여 기능을 조합하는 구조.

**3-1. Component 베이스 클래스** 🔄

인터페이스(pure virtual) 대신 추상 부모 클래스로 설계했다.
라이프사이클 메서드에 기본 빈 구현을 두어, 필요한 컴포넌트만 오버라이드하도록 한다.
`friend class Actor`로 `owner` 주입을 Actor 내부로 캡슐화했다.

```cpp
class Component
{
    friend class Actor;

public:
    virtual void OnAdd();
    virtual void OnRemove();

    Actor& GetOwner() const { return *owner; }

private:
    Actor* owner = nullptr;
};
```

**Actor::AddComponent\<T\>()** — `make_unique`로 생성 후, `move` 전에 raw pointer를 확보해 반환.
소유권은 `unique_ptr`, 참조는 raw pointer.

```cpp
template <typename T>
T* AddComponent()
{
    auto newComponent = std::make_unique<T>();
    T* ptr = newComponent.get();
    ptr->owner = this;
    components.emplace_back(std::move(newComponent));
    return ptr;
}
```

---

## 로드맵

| 단계 | 내용 | 상태 |
|------|------|------|
| 1단계 | 코어 정리 (네이밍, 라이프사이클, Actor 상태) | ✅ 완료 |
| 2단계 | Input (Pressed / Held / Released) | ✅ 완료 |
| 3단계 | Component 시스템 (`AddComponent<T>()`) | 🔄 진행 중 |
| 4단계 | 충돌 시스템 (AABB, BVH) | 🔲 예정 |
| 5단계 | Time 시스템 (DeltaTime, TimeScale) | 🔲 예정 |
| 6단계 | 리소스 관리 (캐싱) | 🔲 예정 |
| 7단계 | Level 고도화 (전환, 스택) | 🔲 예정 |
| 8단계 | 메모리 관리 (Object Pool, Custom Allocator) | 🔲 예정 |
| 9단계 | Renderer (DirectX 11) | 🔲 예정 |
