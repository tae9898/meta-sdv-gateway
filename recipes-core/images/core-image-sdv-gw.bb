SUMMARY = "SDV Gateway image for Raspberry Pi 3"
DESCRIPTION = "Custom Yocto image with CAN-FD/RS485/DoIP gateway application \
for Software-Defined Vehicle diagnostic platform."

inherit core-image

# 필수 패키지
IMAGE_INSTALL += " \
    packagegroup-core-boot \
    packagegroup-core-ssh-openssh \
    ${CORE_IMAGE_EXTRA_INSTALL} \
"

# SDV 게이트웨이 관련 패키지
IMAGE_INSTALL:append = " \
    can-utils \
    iproute2 \
    ethtool \
    sdv-gateway \
    kernel-modules \
"

# 이미지 기능
IMAGE_FEATURES += " \
    debug-tweaks \
    ssh-server-openssh \
    package-management \
"

# 루트파일시스템 크기 여유 (MB)
IMAGE_ROOTFS_EXTRA_SPACE = "512"

# 이미지 타입 (wic.bz2 = SD카드용, ext4 = RAUC 번들용 슬롯 이미지)
IMAGE_FSTYPES = "ext4 wic.bz2 wic.bmap"

# ============================================================
# RAUC A/B OTA 설정
# ============================================================
# A/B 듀얼 rootfs 파티션 레이아웃 (wic/sdimage-dual-raspberrypi.wks.in)
WKS_FILE = "sdimage-dual-raspberrypi.wks.in"

# RAUC + 커널을 rootfs에 포함 (커널이 슬롯별로 업데이트되도록 /boot 가 아닌 rootfs에)
IMAGE_INSTALL:append = " rauc kernel-image"

# 커널을 공유 /boot(FAT) 파티션에서 제거 (각 rootfs 슬롯의 /boot 에서 로드)
RPI_EXTRA_IMAGE_BOOT_FILES:remove = "${KERNEL_IMAGETYPE}"

# 타임존
IMAGE_INSTALL:append = " tzdata"
