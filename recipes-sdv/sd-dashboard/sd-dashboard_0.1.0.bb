SUMMARY = "Predictive Maintenance Web Dashboard (stdlib Python + Chart.js)"
DESCRIPTION = "Reads pm-receiver SQLite and serves a real-time vibration/anomaly \
dashboard over HTTP (port 8080). Pure Python stdlib (http.server + sqlite3) — \
no Flask/pip dependency. Project 4 Phase 2."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
PV = "0.1.0"

SRC_URI = "file://dashboard.py \
           file://sd-dashboard.service"

S = "${WORKDIR}"

inherit systemd

# python3(표준라이브러리 http.server+sqlite3)만 필요. pm-receiver가 만든 DB를 읽음.
RDEPENDS:${PN} = "python3 pm-receiver"

SYSTEMD_SERVICE:${PN} = "sd-dashboard.service"
SYSTEMD_AUTO_ENABLE:${PN} = "enable"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/dashboard.py ${D}${bindir}/dashboard.py
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/sd-dashboard.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${bindir}/dashboard.py ${systemd_system_unitdir}/sd-dashboard.service"
