SUMMARY = "SDV Gateway image for Raspberry Pi 3"
DESCRIPTION = "Custom Yocto image with CAN-FD/RS485/DoIP gateway application \
for Software-Defined Vehicle diagnostic platform."

inherit core-image

# Essential packages
IMAGE_INSTALL += " \
    packagegroup-core-boot \
    packagegroup-core-ssh-openssh \
    ${CORE_IMAGE_EXTRA_INSTALL} \
"

# SDV gateway packages
IMAGE_INSTALL:append = " \
    can-utils \
    iproute2 \
    ethtool \
    sdv-gateway \
    pm-receiver \
    sd-dashboard \
    kernel-modules \
"

# Image features
IMAGE_FEATURES += " \
    debug-tweaks \
    ssh-server-openssh \
    package-management \
"

# Root filesystem extra space (MB)
IMAGE_ROOTFS_EXTRA_SPACE = "512"

# Image types (wic.bz2 = for SD card, ext4 = slot image for RAUC bundle)
IMAGE_FSTYPES = "ext4 wic.bz2 wic.bmap"

# ============================================================
# RAUC A/B OTA configuration
# ============================================================
# A/B dual rootfs partition layout (wic/sdimage-dual-raspberrypi.wks.in)
WKS_FILE = "sdimage-dual-raspberrypi.wks.in"

# Include RAUC + kernel in rootfs (in rootfs, not /boot, so the kernel updates per slot)
IMAGE_INSTALL:append = " rauc kernel-image"

# Remove the kernel from the shared /boot (FAT) partition (load from each rootfs slot's /boot)
RPI_EXTRA_IMAGE_BOOT_FILES:remove = "${KERNEL_IMAGETYPE}"

# Timezone
IMAGE_INSTALL:append = " tzdata"
