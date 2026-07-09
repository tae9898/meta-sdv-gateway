SUMMARY = "Predictive Maintenance UART Receiver (STM32 -> SQLite)"
DESCRIPTION = "Receives framed vibration/anomaly data from STM32 (predictive-maintenance) \
over UART (/dev/ttyACM0) and stores time-series to SQLite. Project 4 Phase 1."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
PV = "0.1.0"

SRC_URI = "file://CMakeLists.txt \
           file://include \
           file://src \
           file://pm-receiver.service"

S = "${WORKDIR}"

inherit cmake systemd

DEPENDS = "sqlite3"
RDEPENDS:${PN} = "sqlite3"

SYSTEMD_SERVICE:${PN} = "pm-receiver.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/pm-receiver.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${systemd_system_unitdir}/pm-receiver.service"
