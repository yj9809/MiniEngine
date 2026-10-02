# MiniEngine

> C++20과 DirectX 11로 Win32 창 생성부터 렌더링 파이프라인, Actor/Component 생명주기까지 직접 구현하며 게임 엔진의 내부 구조를 학습하는 개인 프로젝트입니다.

![OBJ 메시와 WIC 텍스처로 렌더링한 3D 지구](Docs/earth.png)

<sub>OBJ 메시 파싱 · WIC 텍스처 로딩 · WVP 변환 · 자유 시점 카메라를 연결한 데모</sub>

## 현재 상태

기준 커밋: `dbb7a12` (2026-10-01) · 전체 67 commits

현재는 Notion 로드맵의 **5단계 — DX11 렌더러 심화**를 진행하고 있습니다. 텍스처가 적용된 OBJ 모델과 자유 시점 카메라 데모를 완성한 뒤 Material, Lambert Diffuse, Uniform Ambient를 구현했으며, 여러 종류의 광원을 자연스럽게 확장할 수 있도록 LightingSystem의 데이터·컴포넌트·생명주기 경계를 설계하는 중입니다.

완성된 범위와 현재 확장 중인 범위를 구분하면 다음과 같습니다.

| 영역 | 현재 수준 | 상태 |
|---|---|---|
| Win32 런타임 | 창, 메시지 루프, 고정 목표 프레임 루프 | 완료 |
| Actor / Component | `BeginPlay` · `Tick` · `OnDestroy`, 지연 추가/제거, Root Transform | 완료 |
| DX11 기반 | Device · SwapChain · RTV · DSV · Viewport · 상수 버퍼 | 완료 |
| 3D 렌더링 | WVP, 자유 시점 카메라, 깊이 테스트, Indexed Draw | 완료 |
| 에셋 | OBJ 파서, WIC 텍스처 로더, GPU 핸들 관리 | 완료 |
| Material | BaseColor · MainTexture, PS 상수 버퍼 연동 | 완료 |
| 기본 조명 | World Normal, Directional Lambert, Uniform Ambient | 완료 |
| LightingSystem | Light 데이터와 Component 분리, Level 단위 등록·해제 | 설계·구현 중 |
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
- LightingSystem을 추가하면서 “객체 구성”과 “런타임 진입”이 섞여 있던 문제를 발견했고, `Initialize`와 `BeginPlay`의 책임을 분리하는 방향으로 생명주기를 확장하고 있습니다.

### 기반 시스템

| 시스템 | 구현 내용 |
|---|---|
| Math | `Vector2/3/4`, `Matrix4`, 행렬 곱·역행렬·LookAt·PerspectiveFOV |
| Input | Pressed / Held / Released, 마우스 delta, 커서 잠금, Command 바인딩 |
| Time | `QueryPerformanceCounter`, DeltaTime, 이동 평균, clamp, TimeScale, Pause/Resume |
| Camera | WASD 이동, 마우스 우클릭 회전, View / Projection 분리 dirty flag |
| Logging | Debug 빌드용 `OutputDebugStringA` 기반 로그 |

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

기존 OBJ + Texture + Camera 데모는 실행 이미지와 GIF로 확인했습니다. 현재 HEAD는 LightingSystem을 확장하는 작업 중인 스냅샷이며, 2026-10-02 명령줄 빌드에서는 최신 조명 소스의 `Engine.vcxproj` 등록이 아직 반영되지 않아 링크 단계가 남아 있음을 확인했습니다. 이 항목은 LightingSystem 통합 작업과 함께 정리할 예정입니다.

테스트 소스에는 총 89개 케이스가 있습니다.

| 스위트 | 케이스 | 범위 |
|---|---:|---|
| Vector2 / 3 / 4 | 2 / 19 / 18 | 산술, 내적·외적, 정규화, 상수 |
| Matrix4 | 21 | 변환, 역행렬, LookAt, 투영 |
| Time | 17 | 스무딩, clamp, TimeScale, Pause/Resume |
| Level | 12 | Actor 추가·제거, owner, 대량 처리 |

현재 `Tests/Main.cpp`에는 개발 중 빠른 반복 실행을 위한 `TimeTest.*` 필터가 설정되어 있습니다. 다음 통합 체크포인트에서 필터를 제거하고 전체 89개 회귀 테스트를 다시 실행할 예정입니다.

## 주요 설계 판단

| 판단 | 이유 |
|---|---|
| RenderPass 대신 RenderLayer | 현재 구조는 RT 소유·패스 간 SRV 의존성이 아니라 Draw 분류 버킷이므로 실제 책임에 맞게 명명 |
| Light 데이터와 Light Component 분리 | 광원 데이터가 Actor/Component 계층이나 GPU 버퍼 형식에 종속되지 않도록 경계 설정 |
| CPU Light와 GPU Constant Buffer 분리 | 의미 중심의 CPU 데이터와 정렬·전송 중심의 GPU 표현을 독립적으로 변경하기 위함 |
| 프레임 경계에서 Actor 추가·삭제 | Tick/Draw 순회 중 컨테이너 변경과 iterator 무효화 방지 |
| Handle 기반 GPU 리소스 참조 | 게임 오브젝트가 D3D11 COM 객체를 직접 소유하지 않도록 렌더러에 수명 관리 집중 |

## 개발 이력

| 시기 | 주요 결과 | 대표 커밋 |
|---|---|---|
| 2026-03 | Actor/Level 생명주기, Input, Component, AABB 기반 | `8827f0f` · `f17a90f` · `1cddd3d` |
| 2026-04 | DX11 초기화, 수학, RenderCommand, Time, RenderLayer, Camera | `9414065` · `6245c44` · `5595eff` · `acfc93f` |
| 2026-05 | OBJ 메시, MeshRenderer, WIC 텍스처 | `0cf264f` · `5ce9493` |
| 2026-06 | Z-up 전환, 회전 데모, 상대 에셋 경로 | `9963637` · `af58973` |
| 2026-09 | Material, BaseColor, Lambert + Ambient | `b956a29` · `c1ac4c5` · `714511c` · `dc8a878` |
| 2026-10 | Light 데이터 계층, LightingSystem, Component 생명주기 확장 | `5c6a8fe` · `dbb7a12` |

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
| 9~13 | ResourceManager · Level · 메모리 · 직렬화 · 미니 게임 | 예정 |

현재의 바로 다음 목표는 LightingSystem의 end-to-end 흐름을 완성하는 것입니다.

```text
Light Component 등록
→ Level의 LightingSystem이 광원 수집·분류
→ Renderer가 GPU Constant Buffer 형식으로 변환
→ Shader에서 Directional / Point / Spot Light 계산
```

그다음 최신 조명 결과가 드러나는 중간 데모와 GIF를 추가하고 전체 회귀 테스트를 실행합니다.

## 프로젝트 구조

```text
Engine/
├─ Actor/          # Actor 생명주기와 Component 소유
├─ Component/      # Transform, Camera, Mesh, Input, Physics, Light
├─ Core/           # Win32, Input, Time, Log, CollisionSystem
├─ Engine/         # 게임 루프와 렌더러 소유
├─ Level/          # Actor 컨테이너와 프레임 경계 처리
├─ Lighting/       # Light 데이터와 LightingSystem
├─ Math/           # Vector / Matrix
├─ Renderer/       # D3D11, Mesh, Texture, Material, RenderLayer
└─ Shader/         # HLSL
Game/              # 엔진을 사용하는 데모 코드와 에셋
Tests/             # GoogleTest 단위 테스트
Docs/              # README 이미지와 GIF
```

## 문서

- [개발 일지 (repository)](DEVLOG.md)
- [MiniEngine 개발 일지 (Notion)](https://app.notion.com/p/3238d1fa63aa81368587d75d2c0bd738)
- [MiniEngine 개발 로드맵 (Notion)](https://app.notion.com/p/3218d1fa63aa817ab92eddf6e8e86b24)
- [GitHub repository](https://github.com/yj9809/MiniEngine)
