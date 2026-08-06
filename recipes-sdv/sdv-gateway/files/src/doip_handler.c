/**
 * @file doip_handler.c
 * @brief DoIP bidirectional thread (send + receive)
 *
 * doip_tx_thread: pull messages from shared queue → DoIP encapsulation → UDP send
 *                 (unicast reply if a requester exists, else broadcast)
 * doip_rx_thread: receive UDP 13400 → parse DoIP diagnostic → send CAN frame (0x7E0)
 *
 * DoIP header (ISO 13400): [Ver 1B][InvVer 1B][PayloadType 2B][PayloadLen 4B][Payload]
 *   Payload Type 0x8001 = Diagnostic Message
 *   Diagnostic payload = [SrcAddr 2B][TgtAddr 2B][CAN Data N bytes]
 */

#include "doip_handler.h"
#include "gateway.h"

#include <arpa/inet.h>
#include <linux/can.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DOIP_PORT         13400
#define DOIP_VERSION      0x02
#define DOIP_INV_VER      0xFD
#define DOIP_PT_DIAG_MSG  0x8001
#define DOIP_HEADER_LEN   8
#define CAN_IFACE         "can0"
#define CAN_REQUEST_ID    0x7E0   /* OBD-II/UDS diagnostic request ID */

/* Last DoIP requester address (the target doip_tx replies to) */
static struct sockaddr_in g_peer_addr;
static int                g_peer_set = 0;
static pthread_mutex_t    g_peer_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ── DoIP frame builder ─────────────────────────────── */
static size_t build_doip_frame(uint8_t *out, uint16_t ptype,
                                const uint8_t *payload, uint16_t plen)
{
    out[0] = DOIP_VERSION;
    out[1] = DOIP_INV_VER;
    out[2] = (ptype >> 8) & 0xFF;
    out[3] = ptype & 0xFF;
    out[4] = (plen >> 24) & 0xFF;
    out[5] = (plen >> 16) & 0xFF;
    out[6] = (plen >> 8) & 0xFF;
    out[7] = plen & 0xFF;
    if (plen > 0 && payload != NULL) {
        memcpy(&out[DOIP_HEADER_LEN], payload, plen);
    }
    return DOIP_HEADER_LEN + plen;
}

/* gw_message_t → DoIP diagnostic payload ([SrcAddr][TgtAddr][Data]) */
static uint16_t msg_to_diag_payload(const gw_message_t *msg, uint8_t *out,
                                     size_t out_size)
{
    uint16_t src_addr = 0x0E00;
    uint16_t tgt_addr = 0x0E00;
    size_t needed = 4 + msg->dlc;
    if (needed > out_size) needed = out_size;

    out[0] = (src_addr >> 8) & 0xFF;
    out[1] = src_addr & 0xFF;
    out[2] = (tgt_addr >> 8) & 0xFF;
    out[3] = tgt_addr & 0xFF;

    size_t data_len = needed - 4;
    if (data_len > 0) memcpy(&out[4], msg->payload, data_len);
    return (uint16_t)needed;
}

/* ── DoIP send thread (CAN/RS485 → DoIP → Tester) ─── */
void *doip_tx_thread(void *arg)
{
    (void)arg;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[doip-tx] socket failed");
        return NULL;
    }
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in bcast_dest;
    memset(&bcast_dest, 0, sizeof(bcast_dest));
    bcast_dest.sin_family = AF_INET;
    bcast_dest.sin_port = htons(DOIP_PORT);
    bcast_dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    printf("[doip-tx] ready (reply to requester, fallback broadcast)\n");

    uint8_t diag_buf[GW_MAX_PAYLOAD + 4];
    uint8_t doip_buf[DOIP_HEADER_LEN + sizeof(diag_buf)];

    while (g_running) {
        gw_message_t msg;
        if (gw_queue_pop(&g_rx_queue, &msg) != 0) break;

        uint16_t diag_len = msg_to_diag_payload(&msg, diag_buf, sizeof(diag_buf));
        size_t frame_len = build_doip_frame(doip_buf, DOIP_PT_DIAG_MSG,
                                            diag_buf, diag_len);

        /* Target: unicast if requester is known, else broadcast */
        struct sockaddr_in dest = bcast_dest;
        pthread_mutex_lock(&g_peer_mutex);
        if (g_peer_set) dest = g_peer_addr;
        pthread_mutex_unlock(&g_peer_mutex);

        ssize_t sent = sendto(sock, doip_buf, frame_len, 0,
                              (struct sockaddr *)&dest, sizeof(dest));
        if (sent < 0) {
            perror("[doip-tx] sendto failed");
        } else {
            atomic_fetch_add(&g_stats.doip_tx_count, 1);
        }
    }

    printf("[doip-tx] stopping\n");
    close(sock);
    return NULL;
}

/* ── DoIP receive thread (Tester → DoIP → CAN 0x7E0) ─── */
void *doip_rx_thread(void *arg)
{
    (void)arg;

    /* UDP receive socket (13400) */
    int usock = socket(AF_INET, SOCK_DGRAM, 0);
    if (usock < 0) {
        perror("[doip-rx] udp socket failed");
        return NULL;
    }
    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(DOIP_PORT);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(usock, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("[doip-rx] udp bind failed");
        close(usock);
        return NULL;
    }

    /* CAN send socket */
    int csock = gw_open_can_socket(CAN_IFACE, "[doip-rx]");
    if (csock < 0) {
        close(usock);
        return NULL;
    }

    printf("[doip-rx] listening UDP %d → CAN 0x%03X\n", DOIP_PORT, CAN_REQUEST_ID);

    uint8_t buf[DOIP_HEADER_LEN + GW_MAX_PAYLOAD + 8];
    while (g_running) {
        struct sockaddr_in peer;
        socklen_t plen = sizeof(peer);
        ssize_t n = recvfrom(usock, buf, sizeof(buf), 0,
                             (struct sockaddr *)&peer, &plen);
        if (n < DOIP_HEADER_LEN) continue;

        /* DoIP header validation */
        uint16_t ptype = (buf[2] << 8) | buf[3];
        if (buf[0] != DOIP_VERSION || buf[1] != DOIP_INV_VER) continue;
        if (ptype != DOIP_PT_DIAG_MSG) continue;

        /* Remember requester address (doip_tx replies) */
        pthread_mutex_lock(&g_peer_mutex);
        g_peer_addr = peer;
        g_peer_set = 1;
        pthread_mutex_unlock(&g_peer_mutex);

        /* Extract CAN data from diagnostic payload ([src2][tgt2][data...]) */
        uint32_t paylen = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) |
                          ((uint32_t)buf[6] << 8) | buf[7];
        if (paylen < 4) continue;
        uint32_t data_len = paylen - 4;
        if (data_len > 64) data_len = 64;

        /* Send CAN frame (0x7E0) */
        struct canfd_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.can_id = CAN_REQUEST_ID;
        frame.len = (uint8_t)data_len;
        memcpy(frame.data, &buf[DOIP_HEADER_LEN + 4], data_len);

        if (write(csock, &frame, CANFD_MTU) < 0) {
            perror("[doip-rx] can write failed");
        } else {
            printf("[doip-rx] → CAN 0x%03X len=%u (from %s)\n",
                   CAN_REQUEST_ID, data_len, inet_ntoa(peer.sin_addr));
        }
    }

    printf("[doip-rx] stopping\n");
    close(usock); close(csock);
    return NULL;
}
