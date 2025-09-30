# IoT Cloud OTA 관리자 페이지 기술 보고서

## 1. 프로젝트 개요

본 프로젝트는 대규모 IoT 기기 단말 관리를 위한 클라우드 기반 OTA(Over-the-Air) 기술 개발 프로젝트의 프론트엔드 어플리케이션이다. 관리자는 이 페이지를 통해 펌웨어 및 광고를 원격으로 배포하고, 등록된 기기들의 상태를 모니터링하며, 시스템 전반을 관리하는 역할을 수행했다.

## 2. 기술 스택

프로젝트의 안정성과 개발 효율성을 위해 다음과 같은 주요 라이브러리 및 프레임워크를 사용했다.

- **Core:** React, TypeScript, Vite
- **Styling:** Tailwind CSS, Emotion, Material-UI (MUI)
- **State Management:** TanStack Query (React Query)
- **Routing:** React Router
- **HTTP Client:** Axios
- **UI Components:** Lucide React (Icons), React-Toastify, React-Modal, React-Paginate
- **Linting:** ESLint

## 3. 디렉토리 구조

프로젝트는 기능적 분리를 위해 다음과 같은 구조로 구성되었다. 이는 코드의 재사용성을 높이고 유지보수를 용이하게 한다.

```
/
├── public/              # 정적 에셋 (이미지, mock-service-worker 등)
├── src/
│   ├── app/             # 🌍 어플리케이션 진입점, 전역 설정
│   │   ├── App.tsx      # 최상위 앱 컴포넌트
│   │   └── Router.tsx   # 페이지 경로 및 라우팅 구조 정의
│   │
│   ├── pages/           # 📄 라우팅 경로에 해당하는 페이지 컴포넌트
│   │   ├── DashboardPage.tsx
│   │   ├── FirmwareListPage.tsx
│   │   └── ... (각 페이지)
│   │
│   ├── widgets/         # 🧩 여러 페이지에서 재사용되는 복합 UI
│   │   ├── layout/      # 전체 페이지 레이아웃
│   │   └── sidebar/     # 사이드바 및 네비게이션 메뉴
│   │
│   ├── features/        # ✨ 특정 기능을 수행하는 컴포넌트와 로직 집합
│   │   ├── ad_deploy/   # 광고 배포 기능
│   │   ├── firmware_register/ # 펌웨어 등록 기능
│   │   └── ... (기능별 디렉토리)
│   │
│   ├── entities/        # 🧱 어플리케이션의 핵심 도메인 데이터
│   │   ├── device/      # 기기 (API, 타입, UI)
│   │   ├── firmware/    # 펌웨어 (API, 타입, UI)
│   │   ├── advertisement/ # 광고 (API, 타입, UI)
│   │   └── ... (도메인별 디렉토리)
│   │
│   └── shared/          # 🛠️ 여러 곳에서 공통으로 사용되는 모듈
│       ├── api/         # Axios 클라이언트, 공통 API 함수
│       ├── mocks/       # MSW 핸들러, 목업 데이터
│       └── ui/          # Button, Modal 등 공용 UI 컴포넌트
│
├── package.json         # 프로젝트 의존성 및 스크립트
└── vite.config.ts       # Vite 빌드 및 개발 서버 설정
```

## 4. 주요 기능 및 페이지 구현

### 4.1. 라우팅

`react-router`의 `createBrowserRouter`를 사용하여 SPA(Single Page Application)의 라우팅을 구현했다. `/` 경로에 기본 `Layout` 컴포넌트를 배치하고, 그 하위에 각 페이지를 중첩(nested) 라우트로 구성하여 일관된 UI 레이아웃을 유지했다.

- **`src/app/Router.tsx`**: 모든 페이지 경로와 해당 경로에 렌더링될 컴포넌트를 정의했다.
- **`Navigate`**: `/firmware`, `/ads`와 같은 상위 경로로 접근 시, 자동으로 하위 목록 페이지(`.../list`)로 리다이렉트하여 사용자 경험을 개선했다.

### 4.2. 페이지별 상세 구현

#### 4.2.1. 펌웨어 관리 (`/firmware`)

- **펌웨어 목록 (`/firmware/list`)**: `useFirmwareSearch` 커스텀 훅을 통해 펌웨어 목록을 비동기적으로 조회하고, 검색 및 페이지네이션 기능을 구현했다. 목록의 각 항목은 상세 페이지로 연결되는 링크를 제공했다.
- **펌웨어 등록**: `FirmwareListPage`에서 '펌웨어 등록' 버튼 클릭 시 `react-modal`을 사용하여 `FirmwareRegisterForm`을 표시했다. `useFirmwareRegister` 훅을 통해 파일 업로드 및 메타데이터 전송 로직을 처리했다.
- **펌웨어 상세 (`/firmware/:id`)**: `useFirmwareDetail` 훅으로 특정 펌웨어의 상세 정보(버전, 릴리즈 노트, 파일명 등)를 조회했다. 사용자는 이 페이지에서 펌웨어 파일을 다운로드할 수 있다.
- **펌웨어 배포**: `FirmwareDetailPage`에서 '배포하기' 버튼을 클릭하면 `FirmwareDeploy` 모달이 열린다. 사용자는 배포 대상을 리전, 그룹, 또는 개별 기기로 선택할 수 있으며, `useFirmwareDeploy` 훅이 배포 요청을 처리했다.
- **배포 관리 (`/firmware/deployment`)**: `useFirmwareDeploymentSearch` 훅을 통해 펌웨어 배포 이력을 조회하고, 각 배포의 진행 상태(성공, 실패, 진행 중)를 `DeploymentProgressBar`와 `DeploymentStatusBadge`로 시각화하여 보여주었다.

#### 4.2.2. 광고 관리 (`/ads`)

- **광고 목록 (`/ads/list`)**: `useAdSearch` 훅을 사용하여 등록된 광고 목록을 카드 형태로 조회했다. `isSelectionMode` 상태를 통해 일반 모드와 배포 선택 모드를 전환할 수 있도록 구현했다. 선택 모드에서는 최대 3개의 광고를 선택하여 배포할 수 있다.
- **광고 등록**: '광고 등록' 버튼 클릭 시 `AdRegisterForm` 모달을 통해 새로운 광고를 등록했다. `useAdRegister` 훅은 이미지 파일과 바이너리 파일을 S3에 업로드하고 메타데이터를 서버에 전송하는 복합적인 과정을 처리했다.
- **광고 상세 (`/ads/:id`)**: `useAdDetail` 훅으로 특정 광고의 상세 정보와 해당 광고가 적용된 기기 목록을 함께 조회하여 표시했다.
- **광고 배포**: '배포하기' 버튼을 통해 `AdDeploy` 모달을 열고, 선택된 광고들을 특정 리전, 그룹, 기기에 배포하는 기능을 구현했다.
- **배포 관리 (`/ads/deployment`)**: `useAdvertisementDeploymentSearch` 훅을 통해 광고 배포 이력과 상태를 조회했다.

#### 4.2.3. 기기 관리 (`/device`)

- **기기 목록 (`/device`)**: `useDeviceSearch` 훅을 사용하여 등록된 기기 목록을 조회했다. `DeviceFilter` 컴포넌트를 통해 리전 및 그룹별로 기기를 필터링하는 기능을 제공했다.
- **기기 등록**: '디바이스 등록' 버튼 클릭 시 `DeviceRegisterForm` 모달을 표시했다. 사용자가 리전과 그룹을 선택하면 `useDeviceRegister` 훅이 서버에 등록을 요청하고, 반환된 인증 코드를 `Countdown` 컴포넌트와 함께 화면에 표시하여 기기 활성화를 유도했다.
- **기기 상세 (`/device/:deviceId`)**: `useDeviceDetail` 훅으로 특정 기기의 상세 정보(상태, 펌웨어 버전, 소속 그룹/리전 등)와 현재 기기에서 송출 중인 광고 목록을 조회했다. 또한, Grafana 패널을 `iframe`으로 연동하여 실시간 디바이스 메트릭을 시각화했다.

#### 4.2.4. 모니터링 (`/monitoring`)

- Grafana 대시보드의 주요 패널들을 `iframe`으로 임베드하여, 관리자가 페이지 이동 없이 시스템의 핵심 지표(MQTT 메시지, 기기 상태 등)를 실시간으로 모니터링할 수 있도록 구현했다.

## 5. 핵심 로직 및 전략

### 5.1. 서버 상태 관리 (TanStack Query)

- **데이터 페칭 및 캐싱**: `useQuery`를 사용하여 서버 데이터를 가져왔다. `queryKey`에 페이지 번호, 검색어, 필터 값 등을 포함시켜 요청별로 캐시를 관리했다. 이를 통해 동일한 요청에 대해서는 네트워크 호출 없이 캐시된 데이터를 즉시 반환하여 성능을 최적화했다.
- **페이지네이션 UX 개선**: `placeholderData: keepPreviousData` 옵션을 사용하여 페이지 이동 시 이전 데이터를 잠시 보여주어, 데이터가 로드되는 동안 화면 깜빡임 없는 부드러운 사용자 경험을 제공했다.
- **데이터 뮤테이션 및 쿼리 무효화**: 펌웨어/광고 등록, 배포 등 서버 데이터를 변경하는 작업에는 `useMutation`을 사용했다. `onSuccess` 콜백에서 `queryClient.invalidateQueries`를 호출하여 관련된 쿼리(`firmwares`, `ads` 등)를 무효화했다. 이 전략을 통해 데이터 변경 후 별도의 로직 없이 자동으로 최신 목록을 다시 불러와 화면의 데이터 정합성을 유지했다.

### 5.2. 사용자 피드백

- `react-toastify`를 사용하여 사용자 액션에 대한 즉각적인 피드백을 제공했다. 특히 `toast.promise`를 활용하여 `useMutation`으로 처리되는 비동기 작업의 상태(대기, 성공, 실패)를 사용자에게 명확하게 알려주었다.

### 5.3. 전역 상태 관리

- `React Context`와 `localStorage`를 결합하여 화면 잠금(`LockScreen`) 상태를 전역적으로 관리했다. `LockScreenProvider`를 통해 `isLocked` 상태와 `lock`, `unlock` 함수를 제공하며, 사용자가 페이지를 새로고침해도 잠금 상태가 유지되도록 구현했다.

### 5.4. 모달 및 UI 관리

- `react-modal`을 사용하여 펌웨어 등록, 기기 등록, 배포 등 복잡한 사용자 입력을 받는 UI를 모달 창으로 분리하여 구현했다. 이를 통해 메인 페이지의 복잡도를 낮추고 사용자의 집중도를 높였다.
- `tailwindcss`와 `lucide-react`를 적극적으로 활용하여 일관되고 현대적인 디자인 시스템을 구축했다. `shared/ui`에 `Button`, `LabeledValue` 등 재사용 가능한 기본 UI 컴포넌트를 만들어 프로젝트 전반의 생산성을 향상시켰다.
