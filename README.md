# MiniEngine

> C++20과 DirectX 11로 Win32 창 생성부터 렌더링 파이프라인, Actor/Component 생명주기까지 직접 구현하며 게임 엔진의 내부 구조를 학습하는 개인 프로젝트입니다.

![OBJ 메시와 WIC 텍스처로 렌더링한 3D 지구](Docs/earth.png)

<sub>OBJ 메시 파싱 · WIC 텍스처 로딩 · WVP 변환 · 자유 시점 카메라를 연결한 데모</sub>

## 현재 상태

기준 커밋: `363c20f` (2026-10-06) · 전체 75 commits

현재는 Notion 로드맵의 **5단계 — DX11 렌더러 심화**를 진행하고 있습니다. 텍스처가 적용된 OBJ 모델과 자유 시점 카메라 데모, Material, Lambert Diffuse, Uniform Ambient를 구현한 뒤 엔진 서비스와 객체 생명주기를 정리하고 있습니다. `ResourceManager`의 경로별 공유 캐시와 Level 소유 `RenderingSystem`의 등록·해제·정리 기반은 추가했지만, `MeshRendererComponent`가 이 경로를 사용해 매 프레임 `RenderCommand`를 제출하는 연결은 아직 설계·구현 중입니다.

완성된 범위와 현재 확장 중인 범위를 구분하면 다음과 같습니다.

| 영역 | 현재 수준 | 상태 |
|---|---|---|
| Win32 런타임 | 창, 메시지 루프, 고정 목표 프레임 루프 | 완료 |
| Actor / Component | `Initialize` → `BeginPlay` 1회 디스패치, `Tick` · `OnDestroy`, 지연 추가/제거, Root Transform | 기반 완료 / 런타임 생성 경로 보강 중 |
| DX11 기반 | Device · SwapChain · RTV · DSV · Viewport · 상수 버퍼 | 완료 |
| 3D 렌더링 | WVP, 자유 시점 카메라, 깊이 테스트, Indexed Draw | 완료 |
| 에셋 | OBJ 파서, WIC 텍스처 로더, GPU 핸들 관리, 경로별 `ResourceManager` 공유 캐시 | 캐시 기반 구현 / 소비 경로 연결 중 |
| Material | BaseColor · MainTexture, PS 상수 버퍼 연동 | 완료 |
| 기본 조명 | World Normal, Directional Lambert, Uniform Ambient | 완료 |
| LightingSystem | Light 데이터와 Component 분리, Level 단위 등록·해제·정리 | 기반 구현 / 렌더 연동 미완료 |
| RenderingSystem | Level 소유, `MeshRendererComponent` 중복 방지 등록·해제·정리 | 기반 구현 / Submit 연결 미완료 |
| 충돌 | `BoxCollider`, AABB overlap, 등록 컨테이너 | 기반 구현 / 게임 루프 통합 미완료 |

## 무엇을 직접 구현했는가

### 렌더링 파이프라인

```text
MeshRendererComponent
  └─ RenderCommand 제출
      ├─ Buffer / Texture Handle
      ├─ World · View · Projection
      └─ Material BaseColor
          ↓
LayerScheduler
  ├─ OpaqueLayer
  │   ├─ WVP / World / Material / Lighting Constant Buffer
  │   └─ Mesh VS · PS + Texture SRV
  └─ WireframeLayer
      └─ 전용 VS · PS와 Rasterizer State
          ↓
Direct3D 11 DrawIndexed
```

- `IRenderer`와 `D3D11Renderer`를 분리해 게임 코드가 D3D11 구현 세부사항을 직접 참조하지 않도록 구성했습니다.
- 렌더 요청을 `RenderCommand`로 모은 뒤 Opaque/Wireframe 버킷에서 실행합니다. 현재 구조가 Render Target을 소유하는 진짜 RenderPass가 아니라는 점을 확인해 이름을 `RenderLayer`로 정정했습니다.
- OBJ의 position/normal/UV를 파싱하고 중복 정점을 제거해 Vertex/Index Buffer를 생성합니다.
- WIC로 이미지를 RGBA8로 변환하고 Texture2D/SRV를 생성해 픽셀 셰이더에서 샘플링합니다.
- Material의 BaseColor와 Texture를 렌더 명령으로 전달하고, Lambert Diffuse와 Uniform Ambient를 적용하는 셰이더 경로를 구현했습니다.

| Directional Lambert만 적용 | Uniform Ambient 추가 |
|---|---|
| ![Directional Lambert만 적용한 지구](Docs/LambertOnly.png) | ![Uniform Ambient를 추가한 지구](Docs/UniformAmbient.png) |

왼쪽은 광원이 닿지 않는 면이 완전히 어두운 Lambert 단독 결과이고, 오른쪽은 Uniform Ambient를 더해 암부의 기본 밝기를 보완한 결과입니다.

### 오브젝트와 생명주기

```text
Level
  └─ Actor (unique_ptr 소유)
      ├─ TransformComponent (Root)
      ├─ CameraComponent
      ├─ MeshRendererComponent
      ├─ InputComponent
      └─ Collider / Light Component
```

- Actor가 Component를 `unique_ptr`로 소유하고, 외부에서는 비소유 raw pointer로 관찰합니다.
- 순회 중 컨테이너 무효화를 피하기 위해 Actor 추가·삭제를 프레임 경계에서 일괄 처리합니다.
- 반복 `vector::erase`로 인한 O(n²) 삭제를 swap-and-pop으로 변경했습니다. 당시 개발 일지의 10,000개 일괄 삭제 측정은 1046ms에서 12ms로 감소했습니다.
- “객체 구성”과 “런타임 진입”이 섞여 있던 문제를 분리해 `Initialize`와 `BeginPlay` 디스패치를 추가하고, 중복 호출·재진입·순서 역전을 상태로 차단했습니다.
- 현재 Level에 대기 중인 Actor는 프레임 경계에서 `DispatchInitialize()` → `DispatchBeginPlay()` 순으로 진입합니다. 다만 `BeginPlay` 중 새로 생성한 Component가 같은 디스패치 경로를 자동으로 거치는 연결은 아직 보강 대상입니다.

### 기반 시스템

| 시스템 | 구현 내용 |
|---|---|
| Math | `Vector2/3/4`, `Matrix4`, 행렬 곱·역행렬·LookAt·PerspectiveFOV |
| Input | Pressed / Held / Released, 마우스 delta, 커서 잠금, Command 바인딩 |
| Time | `QueryPerformanceCounter`, DeltaTime, 이동 평균, clamp, TimeScale, Pause/Resume |
| Camera | WASD 이동, 마우스 우클릭 회전, View / Projection 분리 dirty flag |
| Logging | Debug 빌드용 `OutputDebugStringA` 기반 로그 |
| Resource | Engine 소유, 메시·텍스처 경로별 `shared_ptr` 캐시, Renderer보다 앞선 정리 순서 |
| System | Level 소유 Lighting / RenderingSystem, 등록·해제와 `EndLevel()` 일괄 정리 |

## 데모

### 축별 회전

| X축 | Y축 | Z축 |
|---|---|---|
| ![X축 회전](Docs/Earth_X축%20회전.gif) | ![Y축 회전](Docs/Earth_Y축%20회전.gif) | ![Z축 회전](Docs/Earth_Z축%20회전.gif) |

`1` / `2` / `3` 키로 각 축 회전을 토글합니다. 현재는 Z-up 왼손 좌표계와 ZYX 오일러 회전을 사용합니다.

### 확인된 한계: 짐벌락

![오일러 회전의 짐벌락](Docs/Earth_짐벌락%20현상.gif)

두 축을 동시에 회전할 때 발생하는 짐벌락을 재현했습니다. 쿼터니언 도입 전까지는 알려진 한계로 유지합니다.

## 빌드와 검증

### 요구 환경

- Windows 10/11
- Visual Studio 2026 Community 또는 MSVC v145 호환 환경
- Windows SDK 10
- x64

### 빌드 절차

1. `Engine.slnx`를 Visual Studio로 엽니다.
2. `x64`와 `Debug` 또는 `Release`를 선택합니다.
3. `Game` 프로젝트를 시작 프로젝트로 설정합니다.
4. 빌드 후 실행 파일 옆으로 복사된 `Asset/`과 `Shader/`를 사용해 실행합니다.

### 검증 현황

기존 OBJ + Texture + Camera 데모와 Lambert / Ambient 결과는 실행 이미지와 GIF로 확인했습니다. 2026-10-05 기록에서는 생명주기·수학·Time의 6개 스위트, 86개 테스트가 모두 통과했습니다. 아래 캡처는 해당 시점의 실행 기록이며, 최신 기준 커밋 `363c20f`의 ResourceManager / RenderingSystem 변경을 포함해 다시 실행한 독립 검증 결과는 아닙니다.

현재 테스트 소스에는 Level 테스트를 포함해 총 98개 케이스가 있습니다.

| 스위트 | 케이스 | 범위 |
|---|---:|---|
| Vector2 / 3 / 4 | 2 / 19 / 18 | 산술, 내적·외적, 정규화, 상수 |
| Matrix4 | 21 | 변환, 역행렬, LookAt, 투영 |
| Time | 17 | 스무딩, clamp, TimeScale, Pause/Resume |
| Lifecycle | 9 | 호출 순서, 1회 보장, 재진입·순서 역전 차단 |
| Level | 12 | Actor 추가·제거, owner, 대량 처리 |

`Tests/Main.cpp`는 현재 필터 없이 `RUN_ALL_TESTS()`를 호출합니다. 제공된 캡처에는 Level 12개 실행 화면이 없으므로, 전체 98개 회귀 테스트와 최신 HEAD 빌드는 다음 통합 체크포인트에서 다시 확인할 예정입니다.

| Lifecycle (9) | Matrix4 (21) |
|---|---|
| ![Lifecycle 테스트 9개 통과](Docs/LifecycleTests.png) | ![Matrix4 테스트 21개 통과](Docs/Matrix4Tests.png) |

| Time (17) | Vector2 (2) |
|---|---|
| ![Time 테스트 17개 통과](Docs/TimeTests.png) | ![Vector2 테스트 2개 통과](Docs/Vector2Tests.png) |

| Vector3 (19) | Vector4 (18) |
|---|---|
| ![Vector3 테스트 19개 통과](Docs/Vector3Tests.png) | ![Vector4 테스트 18개 통과](Docs/Vector4Tests.png) |

## 주요 설계 판단

| 판단 | 이유 |
|---|---|
| RenderPass 대신 RenderLayer | 현재 구조는 RT 소유·패스 간 SRV 의존성이 아니라 Draw 분류 버킷이므로 실제 책임에 맞게 명명 |
| Light 데이터와 Light Component 분리 | 광원 데이터가 Actor/Component 계층이나 GPU 버퍼 형식에 종속되지 않도록 경계 설정 |
| CPU Light와 GPU Constant Buffer 분리 | 의미 중심의 CPU 데이터와 정렬·전송 중심의 GPU 표현을 독립적으로 변경하기 위함 |
| 프레임 경계에서 Actor 추가·삭제 | Tick/Draw 순회 중 컨테이너 변경과 iterator 무효화 방지 |
| Handle 기반 GPU 리소스 참조 | 게임 오브젝트가 D3D11 COM 객체를 직접 소유하지 않도록 렌더러에 수명 관리 집중 |
| Engine 소유 ResourceManager | Level 교체와 무관하게 공유 리소스를 재사용하고 Renderer보다 먼저 캐시를 해제하기 위함 |
| Level 소유 RenderingSystem | 월드별 렌더 대상을 분리하고 `EndLevel()`에서 비소유 등록 목록을 명시적으로 정리하기 위함 |

## 개발 이력

| 시기 | 주요 결과 | 대표 커밋 |
|---|---|---|
| 2026-03 | Actor/Level 생명주기, Input, Component, AABB 기반 | `8827f0f` · `f17a90f` · `1cddd3d` |
| 2026-04 | DX11 초기화, 수학, RenderCommand, Time, RenderLayer, Camera | `9414065` · `6245c44` · `5595eff` · `acfc93f` |
| 2026-05 | OBJ 메시, MeshRenderer, WIC 텍스처 | `0cf264f` · `5ce9493` |
| 2026-06 | Z-up 전환, 회전 데모, 상대 에셋 경로 | `9963637` · `af58973` |
| 2026-09 | Material, BaseColor, Lambert + Ambient | `b956a29` · `c1ac4c5` · `714511c` · `dc8a878` |
| 2026-10 | Light 데이터 계층, 생명주기 디스패치, ResourceManager와 RenderingSystem 기반 | `5c6a8fe` · `dbb7a12` · `96be1c9` · `363c20f` |

## 로드맵

Notion 개발 로드맵을 기준으로 진행합니다.

| 단계 | 내용 | 상태 |
|---|---|---|
| 1~4.8 | 코어 · Input · Component · DX11 기초 · Time · RenderLayer | 완료 |
| 5 | Transform · Camera · Mesh · Texture · Material · LightingSystem | 진행 중 |
| 5.5 | Render Texture + Post-Processing | 예정 |
| 5.6 | LightingSystem 중간 데모와 GIF | 예정 |
| 6 | Deferred Rendering과 실제 RenderPass/RenderGraph | 예정 |
| 7 | PBR | 예정 |
| 8 | AABB 충돌 고도화와 필요 시 BVH | 기반 구현 / 후속 예정 |
| 9~13 | ResourceManager · Level · 메모리 · 직렬화 · 미니 게임 | ResourceManager 기반 구현 / 나머지 예정 |

현재의 바로 다음 목표는 새 서비스와 기존 렌더 경로의 end-to-end 연결을 완성하는 것입니다.

```text
MeshRendererComponent 초기화
→ Level의 ResourceManager에서 Mesh / Texture 공유
→ RenderingSystem 등록·해제
→ 매 프레임 RenderCommand 생성
→ IRenderer::Submit
```

그다음 LightingSystem의 광원 수집·GPU 변환·Shader 계산을 같은 흐름에 연결하고, 최신 조명 데모와 전체 회귀 테스트를 실행합니다.

## 프로젝트 구조

```text
Engine/
├─ Actor/          # Actor 생명주기와 Component 소유
├─ Component/      # Transform, Camera, Mesh, Input, Physics, Light
├─ Core/           # Win32, Input, Time, Log
├─ Engine/         # 게임 루프와 렌더러 소유
├─ Level/          # Actor 컨테이너와 프레임 경계 처리
├─ Math/           # Vector / Matrix
├─ Resource/       # Mesh / Texture 공유 캐시
├─ Renderer/       # D3D11, Mesh, Texture, Material, RenderLayer
├─ System/         # Collision, Lighting, RenderingSystem
└─ Shader/         # HLSL
Game/              # 엔진을 사용하는 데모 코드와 에셋
Tests/             # GoogleTest 단위 테스트
Docs/              # README 이미지와 GIF
```

## 문서

- [개발 일지 (repository)](DEVLOG.md)
- [MiniEngine 개발 일지 (Notion)](https://app.notion.com/p/3238d1fa63aa81368587d75d2c0bd738)
- [MiniEngine 개발 로드맵 (Notion)](https://app.notion.com/p/3218d1fa63aa817ab92eddf6e8e86b24)
- [Actor / Component 생명주기 정리 (Notion)](https://app.notion.com/p/3ec8d1fa63aa81cc8408db4a009803d2)
- [ResourceManager / RenderingSystem 연결 설계 (Notion)](https://app.notion.com/p/3f08d1fa63aa81c4a9d5f0b11473ab0a)
- [GitHub repository](https://github.com/yj9809/MiniEngine)
