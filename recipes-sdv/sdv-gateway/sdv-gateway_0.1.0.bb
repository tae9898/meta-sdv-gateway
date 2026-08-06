SUMMARY = "SDV Multi-protocol Diagnostic Gateway"
DESCRIPTION = "Gateway application routing CAN-FD <-> RS485 <-> DoIP \
for Software-Defined Vehicle diagnostic platform on Raspberry Pi 3."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

PV = "0.1.0"

# Source code is taken directly from the files/ directory within the layer
SRC_URI = "file://CMakeLists.txt \
           file://include \
           file://src \
           file://can-setup.sh \
           file://can0-setup.service \
           file://sdv-gateway.service"

S = "${WORKDIR}"

inherit cmake systemd

DEPENDS = ""

RDEPENDS:${PN} = "can-utils iproute2"

# systemd services (can0-setup: CAN-FD auto-config oneshot, sdv-gateway: gateway main)
SYSTEMD_SERVICE:${PN} = "sdv-gateway.service can0-setup.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

# Install additional files after cmake do_install installs the binary
do_install:append() {
    # CAN init script (called as ExecStart by can0-setup.service)
    install -m 0755 ${WORKDIR}/can-setup.sh ${D}${bindir}/can-setup

    # systemd services
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/can0-setup.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/sdv-gateway.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${bindir}/can-setup ${systemd_system_unitdir}/can0-setup.service ${systemd_system_unitdir}/sdv-gateway.service"
