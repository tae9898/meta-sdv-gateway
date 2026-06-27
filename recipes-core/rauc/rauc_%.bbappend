FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append := "  \
	file://rauc-grow-data-partition.service \
"

# additional dependencies required to run RAUC on the target
# u-boot-env: /etc/fw_env.config (env 위치)
# libubootenv-bin: fw_printenv/fw_setenv (u-boot-fw-utils 제공, 하드웨어 독립)
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
