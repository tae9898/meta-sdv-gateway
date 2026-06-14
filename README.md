# meta-sdv-gw

SDV(Software-Defined Vehicle) 진단 게이트웨이용 Yocto 커스텀 레이어.
Raspberry Pi 3에 CAN-FD/RS485/DoIP 게이트웨이 플랫폼을 올리는 데 사용.

> Yocto Scarthgap 호환

## 레이어 구성

```
meta-sdv-gw/
├── conf/layer.conf                              # 레이어 메타데이터
├── recipes-sdv/
│   └── sdv-gateway/
│       ├── sdv-gateway_0.1.0.bb                 # 게이트웨이 앱 레시피 (cmake + systemd)
│       └── files/                               # 게이트웨이 앱 소스 + 서비스
│           ├── CMakeLists.txt
│           ├── include/                         # C 헤더
│           ├── src/                             # C 소스 (main, can/rs485/doip handler)
│           ├── can-setup.sh                     # CAN 인터페이스 설정
│           └── sdv-gateway.service              # systemd 서비스
└── recipes-core/
    └── images/
        └── core-image-sdv-gw.bb                 # 커스텀 이미지
```

## 제공 레시피

| 레시피 | 설명 |
|--------|------|
| `sdv-gateway` | CAN↔RS485↔DoIP 게이트웨이 앱. systemd 서비스로 자동 시작. 소스는 레이어 내 `files/` 에 포함. |
| `core-image-sdv-gw` | RPi3용 이미지. can-utils, iproute2, openssh, sdv-gateway 포함. |

## 의존 레이어

- `meta` (poky core)
- `meta-python` (meta-openembedded)
- `meta-raspberrypi` (RPi3 보드 지원)

## 사용 방법

### 1. bblayers.conf 에 레이어 추가
```bitbake
BBLAYERS += "/path/to/meta-sdv-gw"
```

### 2. local.conf 설정
```bitbake
MACHINE ??= "raspberrypi3-64"
DISTRO_FEATURES:append = " systemd usrmerge"
LICENSE_FLAGS_ACCEPTED = "synaptics-killswitch"
```

### 3. 빌드
```bash
bitbake core-image-sdv-gw
```

## Mender OTA

Mender OTA 통합은 별도 커밋으로 분리되어 있음. `README-MENDER.md` 참고.
