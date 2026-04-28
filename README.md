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
│   ├── Camera/     # CameraComponent, ViewInfo
│   ├── Physics/    # BoxCollider 등 물리 컴포넌트
│   └── Transform/  # TransformComponent
├── Core/           # Input 시스템, Win32Window, CollisionSystem
├── Engine/         # 엔진 코어, 게임 루프
├── Level/          # 씬(레벨) 관리
├── Math/           # Vector2, Vector3, Vector4 수학 유틸리티 (내적, 외적, 정규화, 상수 포함)
├── Renderer/       # IRenderer, D3D11Renderer
├── Shader/         # HLSL 셰이더 파일 (VertexShader, PixelShader)
└── Setting/        # 엔진 설정 파일
Game/
├── Actor/          # 게임 전용 액터 (QuadActor 등)
└── Main.cpp        # 게임 진입점 (main)
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
| `TimeTest.cpp` | 17 | Update 스무딩/클램프/TotalTime, Pause/Resume |

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

### ✅ Win32 창 시스템

콘솔 기반에서 Win32 창 기반으로 전환. `Win32Window` 클래스가 창 생성과 메시지 처리를 담당하고, Engine이 `unique_ptr`로 소유한다.

**Win32Window** — `Core/Win32Window.h`

- `WNDCLASSEX` 등록 → `CreateWindowEx`로 `HWND` 생성 → `ShowWindow` / `UpdateWindow`
- `WndProc`: `WM_DESTROY`에서 `PostQuitMessage`, `WM_KEYDOWN/UP`에서 `Input::ProcessInputMessage` 연결

**Engine 게임 루프 통합**

- `PeekMessage`를 매 루프마다 비블로킹으로 소진 → `WndProc` 호출
- `WM_QUIT` 감지 시 `isQuit = true`로 루프 종료
- 메시지 처리와 Tick/Draw는 독립적으로 동작 (메시지가 많아도 게임 루프 굶지 않음)

```cpp
while (!isQuit)
{
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT) isQuit = true;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (deltaTime >= oneFrameTime)
    {
        Tick(deltaTime);
        Draw();
        Input::ResetKeyState();
    }
}
```

**Input 시스템 메시지 기반 전환**

- `GetKeyState` 폴링 방식 → `WndProc`에서 메시지 수신 방식으로 전환
- `GetKeyDown(key)` — 이번 프레임에 처음 눌림
- `GetKey(key)` — 누르고 있는 동안
- `GetKeyUp(key)` — 이번 프레임에 뗌
- `ResetKeyState()` — 프레임 끝에 `isKeyDown` / `isKeyUp` 초기화
- `WM_MOUSEMOVE` — 절대 좌표 저장, `GetMousePosition()` / `GetMouseDeltaPosition()` 제공
- 마우스 delta — 프레임 단위 계산 (`SetPreviousMousePosition()`을 프레임 끝에 호출)
- `WM_LBUTTONDOWN/UP`, `WM_RBUTTONDOWN/UP`, `WM_MBUTTONDOWN/UP` — 버튼 상태 3종 구분 (`GetMouseButtonDown` / `GetMouseButton` / `GetMouseButtonUp`)
- 커서 잠금 / 상대 이동 모드 — `SetIsMouseClamped(bool)`으로 게임 코드에서 제어
  - 잠금 시 `ClipCursor` + `ShowCursor(false)`, 매 프레임 `SetCursorPos`로 중앙 리셋
  - `Win32Window::GetWindowRect()` — `GetClientRect` + `ClientToScreen`으로 화면 절대 좌표 반환

---

### ✅ 3단계 — Component 시스템

컴포넌트를 Actor에 붙여 기능을 조합하는 구조.

**3-1. Component 베이스 클래스** ✅

인터페이스(pure virtual) 대신 추상 부모 클래스로 설계했다.
라이프사이클 메서드에 기본 빈 구현을 두어, 필요한 컴포넌트만 오버라이드하도록 한다.
`friend class Actor`로 `owner` 주입을 Actor 내부로 캡슐화했다.

```cpp
class Component
{
    friend class Actor;

public:
    virtual ~Component() = default;

    virtual void OnAdd();
    virtual void OnRemove();
    virtual void Tick(float deltaTime);

    Actor& GetOwner() const { return *owner; }

private:
    Actor* owner = nullptr;
};
```

**Actor::AddComponent\<T\>()** — `make_unique`로 생성 후, `move` 전에 raw pointer를 확보해 반환.
소유권은 `unique_ptr`, 참조는 raw pointer. 컴포넌트가 리스트에 편입된 직후 `OnAdd()`를 호출한다.

```cpp
template <typename T>
T* AddComponent()
{
    auto newComponent = std::make_unique<T>();
    T* ptr = newComponent.get();
    ptr->owner = this;
    components.emplace_back(std::move(newComponent));
    ptr->OnAdd();  // 편입 시 1회 호출
    return ptr;
}
```

**3-2. Component::Tick + Actor::Tick 위임** ✅

`Component`에 `Tick(float deltaTime)`을 추가하고, `Actor::Tick()`에서 보유 컴포넌트를 순차적으로 호출한다.

```cpp
void Actor::Tick(float deltaTime)
{
    for (auto& component : components)
    {
        component->Tick(deltaTime);
    }
}
```

**3-3. ICommand 인터페이스** ✅

커맨드 패턴 기반 입력 처리를 위한 인터페이스. `InputComponent`에서 키 바인딩에 사용된다.

```cpp
namespace Engine
{
    class ICommand
    {
    public:
        virtual ~ICommand() = default;
        virtual void Execute() = 0;
    };
}
```

**3-4. Component RTTI 상속 + GetComponent\<T\>()** ✅

`GetComponent<T>()`에서 타입 조회를 위해 `Component`가 `RTTI`를 상속하도록 변경했다.
`Actor::GetComponent<T>()`는 보유 컴포넌트를 순회해 타입이 일치하는 첫 번째 컴포넌트를 반환한다.

```cpp
template<typename T>
T* GetComponent()
{
    for (auto& com : components)
    {
        if (com->IsTypeOf<T>())
            return com->As<T>();
    }
    return nullptr;
}
```

**3-5. InputComponent 완성** ✅

커맨드 패턴 기반 입력 처리 컴포넌트. 키별로 `ICommand`를 바인딩하고, `Tick()`에서 `Input` 싱글턴의 키 상태를 확인해 `Execute()`를 호출한다.

- `BindKeyDown(int key, unique_ptr<ICommand>)` — 키를 누른 순간
- `BindKey(int key, unique_ptr<ICommand>)` — 키를 누르는 동안
- `BindKeyUp(int key, unique_ptr<ICommand>)` — 키를 뗀 순간

```cpp
void InputComponent::Tick(float deltaTime)
{
    for (auto& [key, command] : keyDownBindings)
        if (Input::GetKeyDown(key)) command->Execute();

    for (auto& [key, command] : keyBindings)
        if (Input::GetKey(key)) command->Execute();

    for (auto& [key, command] : keyUpBindings)
        if (Input::GetKeyUp(key)) command->Execute();
}
```

게임 코드에서 구체 커맨드 클래스를 작성해 바인딩한다. 엔진은 `ICommand` 인터페이스만 알면 되므로 의존성 방향(게임 → 엔진)이 유지된다.

```cpp
// Game 코드
class MoveLeftCommand : public Engine::ICommand
{
public:
    MoveLeftCommand(Player* player) : player(player) {}
    void Execute() override { player->Move(-1, 0); }
private:
    Player* player;
};

// Player::BeginPlay()
auto* input = AddComponent<Engine::InputComponent>();
input->BindKey(VK_LEFT, std::make_unique<MoveLeftCommand>(this));
```

---

### ✅ 4단계 — DX11 렌더러 기초

Win32 창이 이미 있어 HWND를 바로 넘겨 D3D11 초기화 가능한 상태.

**IRenderer 추상 인터페이스** — `GPUInit` / `BeginFrame` / `EndFrame` / `GPUShutdown`

**D3D11Renderer** — `IDXGIFactory2` + `CreateSwapChainForHwnd` + `ComPtr` 기반 구현

- Device + DeviceContext 생성 (Debug 빌드에서 `D3D11_CREATE_DEVICE_DEBUG` 활성화)
- `IDXGIDevice` → `IDXGIAdapter` → `IDXGIFactory2` 경유로 SwapChain 생성 (`CreateSwapChain` 구버전 대신 `CreateSwapChainForHwnd` 사용)
- `RenderTargetView` 생성 및 `OMSetRenderTargets` 바인딩
- Viewport 설정 (`RSSetViewports`)
- `Engine::Draw()`에서 `BeginFrame` / `EndFrame` 호출

**HLSL 셰이더 + 기본 렌더링** ✅

- HLSL 버텍스/픽셀 셰이더 작성 (`VertexShader.hlsl`, `PixelShader.hlsl`)
- `IRenderer`에 `Render()` 순수 가상 함수 추가
- `D3D11Renderer` — `.cso` 로드, 인풋 레이아웃, 버텍스 버퍼 생성, `Render()` 구현
- NDC 좌표 기준 삼각형 화면 출력 확인
- `FAILCHECK` 스택 오버플로우(C6262) 수정 — 에러 처리를 `HandleD3DError` 함수로 분리
- FLIP_DISCARD `Present()` 후 RTV 바인딩 해제 문제 수정 — `BeginFrame()`에서 매 프레임 재바인딩

**4-1. Vector3** ✅

3D 물리 연산을 위한 수학 클래스.

- `float x, y, z` 기반
- 산술: `operator+`, `-`, `*(scalar)`, `/(scalar)`, `==`, `!=`
- `Dot()` (내적), `Cross()` (외적) — 멤버 + static 오버로드
- `LengthSquared()`, `Length()`, `Normalize()`
- 정적 상수: `zero`, `one`, `unitX/Y/Z`

**4-2. Vector4** ✅

동차 좌표(homogeneous coordinates) 및 색상 표현을 위한 수학 클래스.

- `float x, y, z, w` 기반
- 산술: `operator+`, `-`, `*(scalar)`, `/(scalar)`, `==`, `!=`
- `Dot()` (내적) — 멤버 + static 오버로드
- `LengthSquared()`, `Length()`, `Normalize()`
- `ToVector3()` — w 성분 제거 후 Vector3 반환
- `Vector3(v, w)` 생성자로 동차 좌표 변환 지원
- 정적 상수: `zero`, `one`, `unitX/Y/Z`

**4-3. Matrix4** ✅

DX11 기반 4x4 행렬. **행벡터(v*M)**, 왼손 좌표계, **Z-up** (언리얼 기준).

- `union` 방식 — `m00~m33` 이름 접근 + `elements[16]` 순회 접근 공유
- 기본 생성자 — Identity 행렬 / `zero` 정적 상수 (`ZeroMatrix()` 헬퍼로 초기화)
- `Transpose()` — 멤버 + static(`std::swap` 방식)
- `InverseGeneral()` — 여인수 + Adjugate(수반행렬) + `invDet` 방식
- `Inverse()` — 직교행렬 전용 (전치 = 역행렬)
- `operator*(Matrix4)` — 행렬 곱, `operator*(Vector4)` — v*M 벡터 변환
- `TransformPosition(Vector3)` — w=1, 이동+회전+스케일 적용
- `TransformVector(Vector3)` — w=0, 회전+스케일만 적용
- `Translation(x, y, z)` / `Translation(Vector3)` — 이동 행렬 (m30, m31, m32)
- `Scale(s)` / `Scale(x, y, z)` / `Scale(Vector3)` — 크기 행렬 (대각선)
- `RotationX/Y/Z(radians)` — 각 축 회전 (Z-up 왼손 좌표계 기준)
- `Rotation(x, y, z)` / `Rotation(Vector3)` — ZXY 순서 조합 (`Rz * Rx * Ry`)
- `LookAt(eye, at, up)` — 카메라 뷰 행렬 (역행렬 = 전치 → 기저벡터를 열에 배치)
- `PerspectiveFOV(fovY, width, height, nearZ, farZ)` — 원근 투영 (z → [0,1] NDC, m23=1)

**4단계 리팩토링 — GPUInit 분리** ✅

- `GPUInit()`을 `InitDevice` / `InitSwapChain` / `InitRenderTargetView` / `InitViewport` / `InitShaders`로 분리
- Init 함수들 `private`으로 이동, `GPUInit()`은 흐름만 담당

**Constant Buffer (WVP 행렬)** ✅

- `CreateConstantBuffer(UINT byteWidth, ComPtr<ID3D11Buffer>&)` 범용 함수 추가 (`DYNAMIC` + `CPU_ACCESS_WRITE`)
- `Render()`에서 `Map/Unmap`으로 매 프레임 WVP 행렬 업데이트 후 `VSSetConstantBuffers` 바인딩
- `cbuffer WVPConstant : register(b0)` 선언, `mul(float4(pos, 1.0f), wvp)` 적용
- `#pragma pack_matrix(row_major)` — HLSL 기본 열우선 패킹을 행우선으로 변경. CPU 행우선 행렬을 memcpy로 올릴 때 전치 없이 올바르게 읽도록
- `FAILCHECK` 매크로 범용화 — `retval` 인자 추가로 `void` 함수에서도 사용 가능

**Vertex Color** ✅

- `Vertex` 구조체에 RGBA 추가, `InputLayout`에 `COLOR` 슬롯 추가 (`DXGI_FORMAT_R32G32B32A32_FLOAT`)
- `VSInput` / `VSOutput` 구조체 도입으로 VS 입출력 명시화
- PS에서 `VSOutput`을 통째로 받아 `color` 반환 (시맨틱 레지스터 순서 일치)

**IndexBuffer (사각형)** ✅

- 정점 4개 + 인덱스 6개(`0,1,2,0,2,3`)로 사각형 구성
- `IASetIndexBuffer` + `DrawIndexed(6, 0, 0)`으로 전환

**RenderCommand 시스템** ✅

렌더링 호출을 커맨드 객체로 추상화해 큐에 쌓고 일괄 처리하는 구조.

- `BufferHandle` (`uint32_t` typedef) — raw pointer 대신 정수 핸들로 GPU 버퍼 참조
- `RenderCommand` — `vertexBuffer`, `indexBuffer`, `indexCount`, `stride`, `topology` 필드
- `IRenderer`에 `Submit()` / `CreateVertexBuffer()` / `CreateIndexBuffer()` 추가
- `D3D11Renderer` — `bufferMap`(`unordered_map<BufferHandle, ComPtr<ID3D11Buffer>>`)으로 핸들 → 버퍼 조회
- `Render()`에서 `renderCommands` 큐를 순회하며 `DrawIndexed`, 호출 후 `clear()`
- `Level::Draw()` → `Actor::Draw()` 체인 연결, `Engine::Draw()`에서 `mainLevel->Draw()` 호출
- `QuadActor` (Game 전용) — `Init(IRenderer*)` 에서 VB/IB 생성 및 핸들 저장, `Draw()`에서 `Submit()`

```cpp
// QuadActor::Draw()
RenderCommand command;
command.vertexBuffer = vertexBuffer;
command.indexBuffer  = indexBuffer;
command.indexCount   = 6;
command.stride       = sizeof(float) * 7;
command.topology     = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
renderer->Submit(command);
```

**4.8단계 — 경량 Pass Scheduler** ✅

렌더링 명령을 PassType별로 그룹화해 각 패스가 독립적으로 셰이더/래스터라이저 상태를 관리하는 구조.

- `RenderPassType` (`uint8_t` enum) — Opaque / Transparent / Wireframe / UI
- `RenderPass` — Template Method 패턴 베이스 클래스. `Execute()` 루프(커맨드 순회/cbuffer 업데이트/Draw)를 베이스에서 구현. `GetConstantBuffer()` / `Draw()` 순수 가상. `OnPostExecute()` — 루프 종료 후 파이프라인 상태 복구용 no-op 훅
- `PassScheduler` — `unordered_map<RenderPassType, unique_ptr<RenderPass>>`로 패스 등록/조회
- `OpaquePass` — 셰이더, InputLayout, WVP 상수 버퍼 소유. `GetConstantBuffer()` / `Draw()` 구현
- `WireframePass` — OpaquePass와 동일 구조 + `D3D11_FILL_WIREFRAME` 래스터라이저. `OnPostExecute()`에서 `RSSetState(nullptr)` 파이프라인 복구
- `D3D11Renderer::Render()` — `renderCommands`를 PassType별로 그룹화 후 `Prepare → Execute` 순서 실행
- `RenderCommand`에 `passType` 필드 추가 (기본값 `RenderPassType::Opaque`)

---

### ✅ 4.5단계 — Time 시스템

프레임레이트에 독립적인 시간 기반 게임 루프를 위한 시스템.

**Time 클래스 (Static 방식)**

`Time::Get~()` 형식으로 어디서든 접근 가능. 싱글턴 인스턴스 없이 static 멤버로 구현.

- `GetDeltaTime()` — TimeScale이 적용된 deltaTime
- `GetUnscaledDeltaTime()` — TimeScale 무관한 순수 deltaTime
- `GetTotalTime()` — 엔진 시작 후 누적 시간
- `SetTimeScale(float)` / `GetTimeScale()` — 시간 배율 (0.5 = 슬로우모션, 0.0 = 정지)

**프레임 델타 스무딩**

spike(디버거 재개, 포커스 복귀 등) 방지를 위해 두 단계로 처리.

1. **Max clamp** — `SetMaxDeltaTime(oneFrameTime * 10)`으로 극단적 spike 차단. `Engine::Run()`에서 루프 진입 전 설정.
2. **이동 평균** — 최근 10프레임 deltaTime 평균으로 부드럽게 반영.

**일시정지**

- `Pause()` — 현재 TimeScale 저장 후 0으로 설정
- `Resume()` — 저장된 TimeScale 복원
- 슬로우모션 중 `Pause()` 후 `Resume()` 해도 이전 배율로 복원

```cpp
// Engine::Run()
Time::SetMaxDeltaTime(oneFrameTime * 10);  // 루프 전 1회

// 매 프레임
Time::Update(deltaTime);
Tick(Time::GetDeltaTime());  // TimeScale 적용된 값으로 Tick
```

---

### 🔄 5단계 — DX11 렌더러 심화

**5-2. CameraComponent** ✅

언리얼의 `UCameraComponent` + `FMinimalViewInfo` 구조를 참고한 카메라 시스템.

- `ViewInfo` 구조체 — fov / aspect / nearZ / farZ만 보관 (위치/방향은 TransformComponent에서 읽음)
- `CameraComponent` — `OnAdd()`에서 TransformComponent의 `onTransformChanged` 델리게이트에 등록, 위치/방향 변경 시 `dirtyView = true`로 자동 무효화
- dirty 플래그 분리 — `dirtyView`(위치/방향 변경) / `dirtyProjection`(fov/aspect/near/far 변경) 독립 관리
- `LookAt()` — pitch/yaw 기반 Forward 벡터 계산 후 `Matrix4::LookAt(eye, at, up)` 호출
- `Projection()` — `Matrix4::PerspectiveFOV(fov, aspect, nearZ, farZ)` 호출
- `Level::SetMainCamera()` / `GetMainCamera()` — 활성 카메라 등록/조회
- `Actor::Draw()` — 매 프레임 `GetMainCamera()`로 View/Projection 행렬을 공통으로 가져와 protected 멤버에 저장, 파생 Actor가 커맨드에 그대로 사용
- `RenderLayer::UpdateConstantBuffer()` — `world * view * projection` WVP 행렬 계산 후 상수 버퍼에 업로드
- `CameraActor` — WASD(Forward/Right 벡터 기반 이동), 마우스 우클릭 회전(pitch/yaw)

**5-3. Mesh 로딩 준비** 🔄

- MeshVertexShader / MeshPixelShader 신규 작성 (POSITION + NORMAL + UV 포맷)
- InputLayout 교체 — `POSITION` + `NORMAL(float3)` + `TEXCOORD(float2)`로 변경 (OpaqueLayer, WireframeLayer)
- PS 디버그 모드 — Normal을 `* 0.5 + 0.5`로 변환해 [0,1] 색상으로 시각화
- `IRenderer::ReleaseBuffer()` 추가 + `D3D11Renderer` 구현 — Mesh 소멸 시 GPU 버퍼 반환
- Engine 셧다운 순서 버그 수정 — `mainLevel.reset()` → `GPUShutdown()` 순서 보장
- `Mesh.h` 설계 완료 — `LoadFromOBJ(IRenderer*, const char*)` 시그니처, Getter 4종

**5-1. TransformComponent (Root Component)** ✅

모든 Actor의 Root Component로 position / rotationEulerDeg / scale을 통합 관리.
Actor 생성자에서 `AddComponent<TransformComponent>()`로 자동 삽입하고, `rootComponent` 캐시 포인터를 별도 보유한다.

- dirty 플래그 + mutable 캐시 — Transform 변경 시에만 `GetWorldMatrix()` 재계산
- 행렬 합성 순서: Scale * Rotation * Translation (SRT) — HLSL `mul(v, M)` row-vector 방식 기준 (언리얼 실제 코드로 확인)
- deg→rad 변환은 `GetWorldMatrix()` 내부에서만 처리, 저장은 degree 단위 유지
- `parent` 포인터 선언 (구현은 계층 구조 도입 시점에 defer)
- `Actor::position` 멤버 제거, `SetPosition()` / `GetPosition()` → `rootComponent` 위임
- `QuadActor::angle` 제거 → `rootComponent->SetLocalRotationEulerDeg()` 교체
- `OnDestroy()`에서 컴포넌트 전체 순회 후 `OnRemove()` 호출 (기존 dead code 해소)

---

### 🔄 8단계 — 충돌 시스템

AABB 기반 3D 충돌 처리. 먼저 Brute Force로 완성하고, 이후 BVH로 최적화한다.

**8-1. Collider 추상 클래스** ✅

`Component`를 상속하는 추상 기반 클래스. `BoxCollider`, `SphereCollider` 등 모든 콜라이더의 공통 인터페이스를 정의한다.

```cpp
class Collider : public Component
{
    RTTI_DECLARATIONS(Collider, Component)
public:
    virtual bool Overlap(const Collider& other) const = 0;

protected:
    Vector3 center;
    Vector3 worldMin;
    Vector3 worldMax;
    bool isTrigger = false;
};
```

**8-2. BoxCollider** ✅

AABB 방식의 박스 콜라이더. `Tick()`에서 매 프레임 Owner의 월드 좌표로 `worldMin` / `worldMax`를 갱신하고, `Overlap()`에서 세 축을 비교해 충돌을 판정한다.

```cpp
void BoxCollider::Tick(float deltaTime)
{
    Vector3 actorPos = GetOwner().GetPositionF();
    worldMin = actorPos + center - halfSize;
    worldMax = actorPos + center + halfSize;
}

bool BoxCollider::Overlap(const Collider& other) const
{
    Vector3 min = other.GetWorldMin();
    Vector3 max = other.GetWorldMax();

    return worldMin.x <= max.x && worldMax.x >= min.x
        && worldMin.y <= max.y && worldMax.y >= min.y
        && worldMin.z <= max.z && worldMax.z >= min.z;
}
```

- 타입 확인은 `dynamic_cast` 대신 커스텀 RTTI의 `As<T>()` 사용
- `isTrigger` 플래그 — 물리 반응 없이 감지만 하는 트리거 모드 지원 예정
- 디버그 시각화용 꼭짓점 8개(`boxVertices`) — Renderer 붙을 때 활성화 예정

**8-3. CollisionSystem** 🔄

Collider가 직접 CollisionSystem에 Register/Unregister하는 구조. Actor 순회 없이 등록된 Collider만 검사한다.

- `Level`이 `unique_ptr`로 소유 (싱글턴 남용 방지)
- `Register(Collider*)` / `UnRegister(Collider*)` — `Collider::BeginPlay()` / `OnDestroy()`에서 호출 예정
- `UnRegister`는 swap_and_pop 패턴으로 O(1) 처리
- `Tick()` — 충돌 검사 로직 추후 추가 예정

```cpp
void CollisionSystem::UnRegister(Collider* collider)
{
    auto it = std::find(colliders.begin(), colliders.end(), collider);
    if (it != colliders.end())
    {
        std::iter_swap(it, colliders.end() - 1);
        colliders.pop_back();
    }
}
```

---

## 로드맵

| 단계 | 내용 | 상태 |
|------|------|------|
| 1단계 | 코어 정리 (네이밍, 라이프사이클, Actor 상태) | ✅ 완료 |
| 2단계 | Input (Pressed / Held / Released) | ✅ 완료 |
| Win32 | Win32 창 시스템, 메시지 루프 통합 | ✅ 완료 |
| 3단계 | Component 시스템 (`AddComponent<T>()`, `InputComponent`) | ✅ 완료 |
| 4단계 | DX11 렌더러 기초 (IRenderer, D3D11Renderer, SwapChain) | ✅ 완료 |
| 4.5단계 | Time 시스템 (DeltaTime, TimeScale) | ✅ 완료 |
| 4.8단계 | 경량 Pass Scheduler (RenderPass / PassScheduler / OpaquePass / WireframePass) | ✅ 완료 |
| 5단계 | DX11 렌더러 심화 (TransformComponent, 카메라, 메시, 머티리얼, 기본 Phong 조명) | 🔄 진행 중 |
| 5.5단계 | 중간 데모 | 🔲 예정 |
| 6단계 | Deferred Rendering | 🔲 예정 |
| 7단계 | PBR(Physically Based Rendering) | 🔲 예정 |
| 8단계 | 충돌 시스템 (AABB, BVH) | 🔄 진행 중 |
| 9단계 | 리소스 관리 (캐싱) | 🔲 예정 |
| 10단계 | Level 고도화 (전환, 스택) | 🔲 예정 |
| 11단계 | 메모리 관리 (Object Pool, Custom Allocator) | 🔲 예정 |
| 12단계 | 직렬화 (Serialization) | 🔲 예정 |
| 13단계 | 데모 게임 | 🔲 예정 |