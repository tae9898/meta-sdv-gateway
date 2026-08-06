FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append := "  \
	file://rauc-grow-data-partition.service \
"

# additional dependencies required to run RAUC on the target
# u-boot-env: /etc/fw_env.config (env location)
# libubootenv-bin: fw_printenv/fw_setenv (provided by u-boot-fw-utils, hardware-independent)
RDEPENDS:${PN} += "u-boot-env libubootenv-bin"

inherit systemd

SYSTEMD_PACKAGES += "${PN}-grow-data-part"
SYSTEMD_SERVICE:${PN}-grow-data-part = "rauc-grow-data-partition.service"

PACKAGES += "rauc-grow-data-part"

RDEPENDS:${PN}-grow-data-part += "parted"

do_install:append() {
	install -d ${D}${systemd_unitdir}/system/
	install -m 0644 ${WORKDIR}/rauc-grow-data-partition.service ${D}${systemd_unitdir}/system/
}
