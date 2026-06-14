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

# 이미지 타입 (wic = SD 카드용)
IMAGE_FSTYPES = "ext4 wic.bz2 wic.bmap"

# 타임존
IMAGE_INSTALL:append = " tzdata"
