FILESEXTRAPATHS:prepend:rpi := "${THISDIR}/files:"

# Add a mount point for a shared data partition
dirs755 += "/data"

# systemd-networkd: eth0 static IP (10.42.0.2, hotspot gateway .1)
# RPi3 is wired to the PC eth0 hotspot (10.42.0.1). Always gets the same IP.
SRC_URI:append:rpi = " file://10-eth-static.network"
do_install:append:rpi() {
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 ${WORKDIR}/10-eth-static.network ${D}${sysconfdir}/systemd/network/10-eth-static.network
}
