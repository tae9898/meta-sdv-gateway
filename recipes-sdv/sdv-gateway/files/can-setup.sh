#!/bin/sh
# CAN 인터페이스 설정 스크립트
# USB-CAN-FD 어댑터가 연결되면 can0 인터페이스를 설정합니다.
#
# Usage: can-setup [bitrate]

BITRATE=${1:-500000}

# can0 인터페이스가 이미 UP이면 스킵
if ip link show can0 2>/dev/null | grep -q "UP"; then
    echo "[can-setup] can0 already up"
    exit 0
fi

# 인터페이스 down (설정 변경 전)
ip link set can0 down 2>/dev/null

# bitrate 설정 + CAN-FD 활성화 (data phase 2Mbps)
ip link set can0 type can bitrate ${BITRATE} \
    dbitrate 2000000 fd on

# 인터페이스 up
ip link set can0 up

echo "[can-setup] can0 configured: ${BITRATE}bps, CAN-FD data phase 2Mbps"
