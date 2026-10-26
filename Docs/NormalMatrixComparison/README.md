# Normal Matrix 실제 Game 비교 — 2026-10-26

캡처 당시 작업 트리의 기존 `TestMeshActor`와 `world.obj` / `world_giant.jpg`를 사용한 Release x64 실행 캡처다. 새 테스트 모델이나 합성 이미지를 사용하지 않았다. 원본 Engine/Game 소스는 이번 캡처 작업에서 수정하지 않았다.

## 비교 조건

- 해상도: 960×640, 프레임 제한 120.
- Actor 위치: 기존 BeginPlay의 (2,0,0). 회전 Identity, 자동 회전 없음.
- 카메라: (-1.8,0,0), 기존 CameraActor의 기본 자세와 FOV 60도.
- 비균일 스케일: (2,1,0.5). 대조군: (1,1,1).
- 방향광: 기존 DirectionalLightActor를 사용하고 Initialize 이후 강도 0.9, Euler (Pitch=-25°, Yaw=-30°, Roll=0°)로 고정.
- 환경광: 흰색 Sky/Ground 각각 강도 0.06. 어두운 영역에서도 형태를 확인하기 위한 동일한 보조광.
- 두 실행의 Game.exe, 메시, 텍스처, 셰이더는 동일하다. 비교용 Legacy Engine.dll만 b1 업로드 값을 `normalMatrix`에서 `command.worldMatrix`로 바꾼 별도 빌드다.
- 현재 셰이더의 `mul(normal, (float3x3)matrix)`는 이전 셰이더의 `mul(float4(normal,0),world).xyz`와 같은 xyz 계산이다. Legacy는 과거 커밋 전체를 실행한 것이 아니라 **기존 법선 변환을 재현한 비교 빌드**다. 비교는 가역 스케일에 한정한다.

## 결과

| 조건 | 달라진 픽셀 / 614,400 | RGB 8비트 최대 채널 차이 |
| --- | ---: | ---: |
| 기본 스케일 (1,1,1) | 0 | 0 |
| 비균일 스케일 (2,1,0.5) | 39,942 | 123 |

기본 스케일은 두 캡처가 픽셀 단위로 동일하다. 비균일 스케일에서는 동일한 메시 배치·카메라·광원에서 표면 밝기 분포가 달라졌다. 기존 방식은 아래쪽까지 넓게 밝아지고, Normal Matrix 적용 후에는 아래·왼쪽의 어두운 영역이 뚜렷하다. 밝기 감소 자체를 정답 기준으로 사용하지 않는다. 올바른 법선 수치의 근거는 별도로 통과한 실제 생산 VS/OpaqueLayer GPU 테스트이며, 이번 화면은 실제 Game 적용 결과의 시각 증거다.

앞선 Debug/Release 각각 68개 관련 테스트 통과와 Hardware/WARP 검증은 `Bin/{Debug,Release}/Tests/normal-matrix-results.xml`에 있다. 이번 캡처는 Game 실제 실행이며 전체 테스트 재실행이나 성능 측정은 아니다.

## 산출물과 출처

- `index.html`: 적용 전/후 슬라이더와 기본 스케일 대조군.
- `legacy-nonuniform.png`, `current-nonuniform.png`: 비균일 스케일의 실제 창 캡처.
- `legacy-uniform.png`, `current-uniform.png`: 기본 스케일 대조군.
- `pixel-comparison.json`: RGB 픽셀 차이 측정값. 화면 전체를 기준으로 하며 물체 영역만의 평균이 아니다.
- `manifest.json`: 기준 커밋, 캡처 시각, 사용한 소스·실행 파일·셰이더·이미지 SHA-256. 미커밋 변경을 포함하므로 기준 커밋만으로 재현되는 결과가 아니다.
- `fixture/`: 비교 장면과 빌드·캡처·측정 스크립트의 보존본. 원본 작업 트리의 구현은 덮어쓰지 않는다.

Windows 캡처 도구의 MXC 실행기 오류로, 직접 시작한 Game 프로세스의 클라이언트 창만 `PrintWindow(PW_CLIENTONLY | PW_RENDERFULLCONTENT)`로 저장했다. 캡처 픽셀은 보정·합성하지 않았다. 실행한 Game 프로세스는 각 캡처 후 종료했다.

## 재현 범위와 보존본의 제한

이 자료는 Normal Matrix 단독 구현 직후의 기록이다. 이후 World/Camera Position 전달에서 b1이 64바이트 NormalConstant에서 128바이트 ObjectConstant로 변경됐다. 따라서 현재 커밋에서 아래 보존 스크립트를 그대로 실행할 수 없다. Prepare.ps1은 옛 치환 대상이 없으면 중단하여 동일한 두 빌드를 전후 비교로 오인하지 않게 한다.

당시 캡처에는 커밋되지 않은 DirectionalLightActor의 자동 회전 제거도 포함됐다. manifest의 기준 커밋만 체크아웃해서는 당시 작업 트리가 복원되지 않으며, 당시 실행 파일·작업 상태 또는 그에 맞는 별도 재현 구성이 필요하다. 저장된 PNG와 수치는 당시 검증 근거이고, 이후 위치 전달 변경의 Game 화면 검증으로 사용하지 않는다.

아래는 당시 작업 상태에서 사용한 절차다. 스크립트는 보존 목적으로 포함했으며, 이번 커밋 준비에서는 새 빌드·캡처를 수행하지 않았다.

## 당시 재현 절차

당시 구현의 Release Engine 빌드를 먼저 준비한다. `fixture`의 파일들을 저장소 `Bin/NormalMatrixComparison/`으로 복사하고 저장소 루트에서 실행한다. `Prepare.ps1`은 원본 Game 프로젝트와 Actor 소스를 재사용하는 별도 프로젝트를 만들며 원본 프로젝트·소스를 수정하지 않는다.

```powershell
& .\Bin\NormalMatrixComparison\Prepare.ps1
& .\Bin\NormalMatrixComparison\Capture.ps1 -Mode Legacy
& .\Bin\NormalMatrixComparison\Capture.ps1 -Mode Current
& .\Bin\NormalMatrixComparison\Capture.ps1 -Mode Legacy -Uniform
& .\Bin\NormalMatrixComparison\Capture.ps1 -Mode Current -Uniform
& .\Bin\NormalMatrixComparison\Measure.ps1
```

재현은 이미지와 manifest를 새 실행 결과로 갱신한다. 캡처 동안 입력을 주지 않고 두 비교에서 같은 작업 트리를 유지한다. 프로젝트 기준 협업 규칙에 따라 Notion 체크와 PROGRESS.md는 자동 갱신하지 않았다.
