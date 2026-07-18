/**
 * @file can_handler.c
 * @brief SocketCAN 수신 스레드 구현
 *
 * USB-CAN-FD 어댑터 → SocketCAN (can0) → gw_queue_push()
 */

#include "can_handler.h"
#include "gateway.h"

#include <linux/can.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CAN_IFACE "can0"

void *can_rx_thread(void *arg)
{
    (void)arg;

    int sock = gw_open_can_socket(CAN_IFACE, "[can]");
    if (sock < 0) {
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
        msg.timestamp_ns = gw_get_monotonic_ns();

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
