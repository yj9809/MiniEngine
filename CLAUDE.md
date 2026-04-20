# MiniEngine — Claude Code 가이드

## 프로젝트 개요

C++ 기반 미니 게임 엔진. 목표: 엔진 프로그래머 역량 향상을 위한 학습용 엔진 개발.
언리얼 엔진의 설계 철학을 참고하여 구조를 잡는다.

---

## 프로젝트 구조

```
Engine/
├── Actor/
│   ├── Actor.h
│   └── Actor.cpp
├── Common/
│   ├── Common.h       # ENGINE_API 매크로, FAILCHECK, HandleD3DError
│   └── RTTI.h
├── Component/
│   ├── Component.h
│   ├── Component.cpp
│   ├── ICommand.h
│   ├── InputSystem/
│   │   ├── InputComponent.h
│   │   └── InputComponent.cpp
│   └── Physics/
│       ├── Collider.h
│       ├── BoxCollider.h
│       └── BoxCollider.cpp
├── Core/
│   ├── Input.h
│   ├── Input.cpp
│   ├── Win32Window.h
│   ├── Win32Window.cpp
│   ├── CollisionSystem.h
│   └── CollisionSystem.cpp
├── Engine/
│   ├── Engine.h
│   └── Engine.cpp
├── Level/
│   ├── Level.h
│   └── Level.cpp
├── Math/
│   ├── Vector2.h / .cpp
│   ├── Vector3.h / .cpp
│   ├── Vector4.h / .cpp
│   └── Matrix4.h / .cpp
├── Renderer/
│   ├── IRenderer.h
│   ├── D3D11Renderer.h
│   └── D3D11Renderer.cpp
├── Shader/
│   ├── VertexShader.hlsl
│   └── PixelShader.hlsl
└── Setting/
    └── Settings.txt
```

---

## 설계 원칙 (반드시 준수)

### 메모리 관리
- 소유권이 있는 포인터는 반드시 `unique_ptr` 사용
- raw pointer(`*`)는 소유권 없는 참조(observer)에만 허용
- `new` / `delete` 직접 사용 금지

### 네이밍 컨벤션 (언리얼 스타일)
| 항목 | 규칙 | 예시 |
|------|------|------|
| 클래스/구조체 | PascalCase | `Actor`, `ScreenBuffer` |
| 함수 | PascalCase | `BeginPlay()`, `Tick()` |
| 멤버 변수 | camelCase | `isActive`, `mainLevel` |
| 생명주기 함수 | 언리얼 명칭 사용 | `BeginPlay`, `Tick`, `OnDestroy`, `OnCollision` |

> ⚠️ `update()` 같은 소문자 함수명은 사용하지 않는다. 반드시 `Update()` 또는 `Tick()`으로 통일.

### 설정 파일
- `std::ifstream` 사용 (C 스타일 `fopen_s` 금지)

### 싱글턴
- 남용 금지. `Input`, `Renderer`에만 제한적으로 허용
- 그 외 시스템은 싱글턴으로 만들지 않는다

### 의존성 방향
- **엔진 코드는 게임 코드에 의존하면 안 된다**
- 의존 방향: 게임 → 엔진 (단방향)

---

## 개발 로드맵

> 상세 내용은 Notion 로드맵 참고. 여기선 현재 진행 상태만 기록.

| 단계 | 내용 | 상태 |
|------|------|------|
| 1단계 | 코어 정리 (네이밍, 라이프사이클, Actor 상태) | ✅ 완료 |
| 2단계 | Win32 창 생성 + Input 시스템 | ✅ 완료 |
| 3단계 | Component 시스템 (AddComponent, InputComponent) | ✅ 완료 |
| **4단계** | **DX11 렌더러 기초** | **🔄 진행 중** |
| 4.5단계 | Time 시스템 (DeltaTime, TimeScale) | ✅ 완료 |
| 4.8단계 | 경량 Pass Scheduler | ⬜ 예정 |
| 5단계 | DX11 렌더러 심화 (TransformComponent, 카메라, 메시, 머티리얼, Phong) | ⬜ 예정 |
| 5.5단계 | 중간 데모 체크포인트 | ⬜ 예정 |
| 6단계 | Deferred Rendering | ⬜ 예정 |
| 7단계 | PBR | ⬜ 예정 |
| 8단계 (취업 후) | 충돌 시스템 (AABB, BVH) | ⬜ 예정 |
| 9단계 | 리소스 관리 | ⬜ 예정 |
| 10단계 | Level 고도화 | ⬜ 예정 |
| 11단계 | 메모리 관리 (Object Pool, Custom Allocator) | ⬜ 예정 |
| 12단계 | 직렬화 | ⬜ 예정 |
| 13단계 | 데모 게임 | ⬜ 예정 |

### 4단계 현재 진행 상황
- [x] D3D11 초기화 (Device, SwapChain, RTV, Viewport)
- [x] IRenderer 추상 인터페이스
- [x] 삼각형 NDC 출력
- [x] Vector3 / Vector4 / Matrix4 구현
- [x] GPUInit 리팩토링 (Init 함수 분리)
- [x] Constant Buffer 구조 설계 (WVP 행렬 GPU 전달)
- [x] Vertex Color (VSInput/VSOutput 구조체, InputLayout COLOR)
- [x] IndexBuffer 도입 (사각형, DrawIndexed)
- [ ] Constant Buffer — 실제 WVP 행렬 적용 (카메라 시스템 이후)
- [x] RenderCommand 시스템
- [x] DeltaTime 회전 데모 (QuadActor, worldMatrix per command)

---

## 코드 작성 시 주의사항

- 코드 수정 또는 추가 시 **전체 클래스 단위**로 제공한다 (일부 스니펫 금지)
- 헤더(`.h`)와 소스(`.cpp`)를 분리해서 작성한다
- 새 기능 추가 전, 위 버그 목록이 해결되었는지 먼저 확인한다
- STL 사용 가능. 단, 추후 Custom Allocator 도입을 고려해 `std::vector` 등의 교체 가능성을 열어둔다

---

## 참고

- 장기 목표: 언리얼 엔진 학습으로 이어지는 브릿지 프로젝트
- 로드맵 상세는 Notion에 별도 정리됨

## Skill routing

When the user's request matches an available skill, ALWAYS invoke it using the Skill
tool as your FIRST action. Do NOT answer directly, do NOT use other tools first.
The skill has specialized workflows that produce better results than ad-hoc answers.

Key routing rules:
- Product ideas, "is this worth building", brainstorming → invoke office-hours
- Bugs, errors, "why is this broken", 500 errors → invoke investigate
- Ship, deploy, push, create PR → invoke ship
- QA, test the site, find bugs → invoke qa
- Code review, check my diff → invoke review
- Update docs after shipping → invoke document-release
- Weekly retro → invoke retro
- Design system, brand → invoke design-consultation
- Visual audit, design polish → invoke design-review
- Architecture review → invoke plan-eng-review
- Save progress, checkpoint, resume → invoke checkpoint
- Code quality, health check → invoke health