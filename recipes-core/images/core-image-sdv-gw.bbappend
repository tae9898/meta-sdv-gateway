# ============================================================
# Mender OTA 통합 (core-image-sdv-gw.bbappend)
# ============================================================
#
# 주의: 이 bbappend가 동작하려면 meta-mender가 bblayers.conf에 있어야 함.
# Mender 없이 베이스 이미지만 빌드하려면 이 파일을 삭제 (또는 commit 2 revert).
#
# 설정 단계는 README-MENDER.md 참고.

# Mender 풀 기능 (클라이언트 + A/B 업데이트)
inherit mender-full

# Mender 아티팩트 이미지 타입 (OTA 배포용 .mender 파일 생성)
IMAGE_FSTYPES:append = " ${MENDER_ARTIFACT_IMAGE_FSTYPES}"

# 게이트웨이 이미지에 Mender 클라이언트 포함
IMAGE_INSTALL:append = " mender mender-artifact mender-artifact-image"

# RPi3 부트 파티션 (Mender 레이아웃용)
MENDER_BOOT_PART_SIZE_MB = "16"
