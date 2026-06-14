# Mender OTA 통합 가이드

`core-image-sdv-gw` 이미지에 Mender OTA(A/B 무결함 업데이트)를 통합하는 방법.
본 통합은 meta-sdv-gw의 **별도 커밋**으로 분리되어 있어, 제거하면 Mender 없는 베이스 이미지로 복귀 가능.

## 사전 준비

### 1. meta-mender 클론
```bash
cd yocto
git clone https://github.com/mendersoftware/meta-mender.git -b scarthgap
```

### 2. bblayers.conf 에 Mender 레이어 추가
```bitbake
BBLAYERS += " \
  ${TOPDIR}/../meta-mender/meta-mender-core \
"
```

## Mender 서버 설정

### Hosted Mender (무료, 기기 10대까지)
1. https://hosted.mender.io 가입
2. Settings → Tenant token 복사

### local.conf 설정
```bitbake
MENDER_SERVER_URL = "https://hosted.mender.io"
MENDER_TENANT_TOKEN = "<복사한 토큰>"
```

> 스테이징/자체 호스팅 서버를 쓰면 `MENDER_SERVER_URL`을 해당 주소로 변경.

## 빌드

```bash
bitbake core-image-sdv-gw
```

빌드 산출물(`tmp/deploy/images/raspberrypi3-64/`):
- `core-image-sdv-gw-*.rootfs.wic.bz2` — 최초 SD 카드 플래시용 (전체 이미지)
- `core-image-sdv-gw-*.mender` — OTA 업데이트 아티팩트 (이것만 서버에 업로드)

## OTA 업데이트 흐름

```
1. 최초: SD 카드에 .wic.bz2 플래시 → 부팅 (A 파티션 활성)
2. 코드 수정 → bitbake → .mender 아티팩트 생성
3. .mender를 Hosted Mender에 업로드
4. RPi3가 Mender 서버에서 아티팩트 다운로드 → B 파티션에 설치
5. 재부팅 → B에서 부팅 → 성공 시 A/B 전환 확정
   (실패 시 자동으로 A로 롤백)
```

## Mender 비활성화 (베이스 이미지만)

이 파일들을 삭제하거나 commit을 revert:
```
recipes-core/images/core-image-sdv-gw.bbappend
README-MENDER.md
```
그러면 Mender 없는 일반 단일 파티션 이미지로 빌드됨.

## 제약 / 참고

- Mender A/B 업데이트는 RPi3의 U-Boot 부트로더 설정과 연동됨 (meta-mender가 자동 처리)
- `MENDER_TENANT_TOKEN`은 local.conf에 평문으로 들어가므로 **git에 커밋 금지**
- 포트폴리오 데모이므로 Hosted Mender 무료 티어 사용 가정
