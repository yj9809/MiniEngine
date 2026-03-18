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
│   ├── Common.h
│   └── RTTI.h
├── Engine/
│   ├── Engine.h
│   └── Engine.cpp
├── Level/
│   ├── Level.h
│   └── Level.cpp
├── Math/
│   ├── Vector2.h
│   └── Vector2.cpp
├── Renderer/
│   ├── ScreenBuffer.h
│   └── ScreenBuffer.cpp
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

## 현재 알려진 버그 및 미완성 항목

### 치명적 버그
- [ ] `Engine.cpp` 게임 루프에서 `Start()`를 매 프레임 호출하고 있음
  - 수정 방향: `BeginPlay()` + `hasBeganPlay` 플래그로 교체

### 네이밍 불일치
- [ ] `update()` → `Update()` / `Tick()`으로 통일

### 미완성 코드
- [ ] `ScreenBuffer.h` — `Draw(CHAR_INFO)` 세미콜론 누락, 매개변수 이름 없음
- [ ] `Engine::Draw()` — 구현 비어있음

---

## 개발 로드맵

### 1단계 — 코어 정리 (현재)
- `BeginPlay()` / `hasBeganPlay` 플래그 도입
- `isActive` / `destroyRequested` 플래그 분리
- `OnDestroy()` 콜백 추가

### 2단계 — Renderer
- 더블 버퍼링
- `RenderCommand` 큐
- `Submit()` / `Draw()` 구조 분리
- Color 지원

### 3단계 — Input 시스템
- `Pressed` / `Held` / `Released` 상태 구분 ✅

### 4단계 — Component 시스템
- `AddComponent<T>()` 템플릿 ✅
- `Component::Tick(float deltaTime)` 추가
- `Actor::Tick()`에서 보유 컴포넌트의 `Tick()` 순차 호출
- `ICommand` 인터페이스 추가 (`Execute()`)
- `InputComponent` 구현 — 커맨드 패턴 기반 입력 처리
  - `BindKeyDown(int key, unique_ptr<ICommand>)`
  - `BindKey(int key, unique_ptr<ICommand>)`
  - `BindKeyUp(int key, unique_ptr<ICommand>)`
  - `Tick()`에서 `Input`의 키 상태 확인 후 커맨드 `Execute()` 호출

### 5단계 — 충돌 시스템
- AABB
- `CollisionSystem`
- `OnCollision()` 콜백

### 6단계 — Time 시스템
- `TimeScale`, `ElapsedTime`

### 7단계 — 리소스 관리
- 캐싱 기반

### 8단계 — Level 고도화
- Level 전환 / 스택

### 9단계 — 메모리 관리
- Object Pool
- Custom Allocator

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