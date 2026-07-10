FILESEXTRAPATHS:prepend:rpi := "${THISDIR}/files:"

# Add a mount point for a shared data partition
dirs755 += "/data"

# systemd-networkd: eth0 고정 IP (10.42.0.2, 핫스팟 게이트웨이 .1)
# RPi3 는 PC eth0 핫스팟(10.42.0.1)에 유선 연결. 항상 같은 IP 보장.
SRC_URI:append:rpi = " file://10-eth-static.network"
do_install:append:rpi() {
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 ${WORKDIR}/10-eth-static.network ${D}${sysconfdir}/systemd/network/10-eth-static.network
}
