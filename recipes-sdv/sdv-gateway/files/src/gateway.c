/**
 * @file gateway.c
 * @brief 공유 큐 및 통계 구현
 */

#include "gateway.h"

#include <errno.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

/* ── Ring Buffer Queue ─────────────────────────────────────── */

int gw_queue_init(gw_queue_t *q)
{
    memset(q, 0, sizeof(*q));
    if (pthread_mutex_init(&q->mutex, NULL) != 0) {
        return -1;
    }
    if (pthread_cond_init(&q->cond_not_empty, NULL) != 0) {
        pthread_mutex_destroy(&q->mutex);
        return -1;
    }
    return 0;
}

void gw_queue_destroy(gw_queue_t *q)
{
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond_not_empty);
}

int gw_queue_push(gw_queue_t *q, const gw_message_t *msg)
{
    pthread_mutex_lock(&q->mutex);

    int next = (q->tail + 1) % GW_QUEUE_DEPTH;
    if (next == q->head) {
        /* 큐 full — 가장 오래된 메시지 drop */
        q->head = (q->head + 1) % GW_QUEUE_DEPTH;
    }

    q->buffer[q->tail] = *msg;
    q->tail = next;

    pthread_cond_signal(&q->cond_not_empty);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

int gw_queue_pop(gw_queue_t *q, gw_message_t *msg)
{
    pthread_mutex_lock(&q->mutex);

    while (q->head == q->tail && g_running) {
        pthread_cond_wait(&q->cond_not_empty, &q->mutex);
    }

    if (!g_running && q->head == q->tail) {
        pthread_mutex_unlock(&q->mutex);
        return -1;  /* 종료 신호 */
    }

    *msg = q->buffer[q->head];
    q->head = (q->head + 1) % GW_QUEUE_DEPTH;

    pthread_mutex_unlock(&q->mutex);
    return 0;
}

/* ── Stats ─────────────────────────────────────────────────── */

int gw_stats_init(gw_stats_t *s)
{
    memset(s, 0, sizeof(*s));
    return pthread_spin_init(&s->lock, PTHREAD_PROCESS_PRIVATE);
}

void gw_stats_destroy(gw_stats_t *s)
{
    pthread_spin_destroy(&s->lock);
}

void gw_stats_print(gw_stats_t *s)
{
    pthread_spin_lock(&s->lock);
    printf("[stats] CAN RX: %lu  RS485 RX: %lu  DoIP TX: %lu\n",
           (unsigned long)s->can_rx_count,
           (unsigned long)s->rs485_rx_count,
           (unsigned long)s->doip_tx_count);
    pthread_spin_unlock(&s->lock);
}

/* ── Utilities ─────────────────────────────────────────────── */

uint64_t gw_get_monotonic_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int gw_open_can_socket(const char *ifname, const char *tag)
{
    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        fprintf(stderr, "%s socket: %s\n", tag, strerror(errno));
        return -1;
    }

    int enable = 1;
    if (setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable, sizeof(enable)) < 0) {
        fprintf(stderr, "%s setsockopt CAN_RAW_FD_FRAMES: %s\n", tag, strerror(errno));
        close(sock);
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        fprintf(stderr, "%s ioctl SIOCGIFINDEX (%s up?): %s\n", tag, ifname, strerror(errno));
        close(sock);
        return -1;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "%s bind: %s\n", tag, strerror(errno));
        close(sock);
        return -1;
    }

    return sock;
}
