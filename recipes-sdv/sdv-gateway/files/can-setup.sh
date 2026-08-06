#!/bin/sh
# CAN interface setup script
# Configures the can0 interface when a USB-CAN-FD adapter is connected.
#
# Usage: can-setup [bitrate]

BITRATE=${1:-500000}

# Skip if can0 interface is already UP
if ip link show can0 2>/dev/null | grep -q "UP"; then
    echo "[can-setup] can0 already up"
    exit 0
fi

# Interface down (before changing config)
ip link set can0 down 2>/dev/null

# Set bitrate + enable CAN-FD (data phase 2Mbps)
ip link set can0 type can bitrate ${BITRATE} \
    dbitrate 2000000 fd on

# Interface up
ip link set can0 up

echo "[can-setup] can0 configured: ${BITRATE}bps, CAN-FD data phase 2Mbps"
