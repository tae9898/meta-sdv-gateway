/**
 * @file can_handler.c
 * @brief SocketCAN 수신 스레드 구현
 *
 * USB-CAN-FD 어댑터 → SocketCAN (can0) → gw_queue_push()
 */

#include "can_handler.h"
#include "gateway.h"

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define CAN_IFACE "can0"

static uint64_t get_monotonic_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void *can_rx_thread(void *arg)
{
    (void)arg;

    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        perror("[can] socket failed");
        return NULL;
    }

    /* CAN-FD 모드 활성화 */
    int enable = 1;
    if (setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable, sizeof(enable)) < 0) {
        perror("[can] setsockopt CAN_RAW_FD_FRAMES failed");
        close(sock);
        return NULL;
    }

    /* 인터페이스 바인드 */
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;

    struct ifreq ifr;
    strncpy(ifr.ifr_name, CAN_IFACE, IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("[can] ioctl SIOCGIFINDEX failed (is can0 up?)");
        close(sock);
        return NULL;
    }
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[can] bind failed");
        close(sock);
        return NULL;
    }

    printf("[can] listening on %s\n", CAN_IFACE);

    /* 수신 루프 */
    struct canfd_frame frame;
    while (g_running) {
        int nbytes = read(sock, &frame, sizeof(frame));
        if (nbytes < 0) {
            if (!g_running) break;
            perror("[can] read error");
            continue;
        }

        /* 큐에 push */
        gw_message_t msg;
        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_TYPE_CAN;
        msg.can_id = frame.can_id & CAN_EFF_MASK;
        msg.dlc = (uint8_t)frame.len;
        msg.timestamp_ns = get_monotonic_ns();

        size_t copy_len = frame.len;
        if (copy_len > GW_MAX_PAYLOAD) {
            copy_len = GW_MAX_PAYLOAD;
        }
        memcpy(msg.payload, frame.data, copy_len);

        gw_queue_push(&g_rx_queue, &msg);
        atomic_fetch_add(&g_stats.can_rx_count, 1);
    }

    printf("[can] stopping\n");
    close(sock);
    return NULL;
}
