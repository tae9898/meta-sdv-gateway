SUMMARY = "SDV Multi-protocol Diagnostic Gateway"
DESCRIPTION = "Gateway application routing CAN-FD <-> RS485 <-> DoIP \
for Software-Defined Vehicle diagnostic platform on Raspberry Pi 3."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

PV = "0.1.0"

# 소스코드는 레이어 내 files/ 디렉토리에서 직접 가져옴
SRC_URI = "file://CMakeLists.txt \
           file://include \
           file://src \
           file://can-setup.sh \
           file://sdv-gateway.service"

S = "${WORKDIR}"

inherit cmake systemd

DEPENDS = ""

RDEPENDS:${PN} = "can-utils iproute2"

# systemd 서비스
SYSTEMD_SERVICE:${PN} = "sdv-gateway.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

# cmake do_install로 바이너리 설치 후 추가 파일 설치
do_install:append() {
    # CAN 초기화 스크립트
    install -m 0755 ${WORKDIR}/can-setup.sh ${D}${bindir}/can-setup

    # systemd 서비스
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/sdv-gateway.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${bindir}/can-setup ${systemd_system_unitdir}/sdv-gateway.service"
