/**
 * @file gateway.c
 * @brief 공유 큐 및 통계 구현
 */

#include "gateway.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

void gw_stats_print(const gw_stats_t *s)
{
    pthread_spin_lock((pthread_spinlock_t *)&s->lock);
    printf("[stats] CAN RX: %lu  RS485 RX: %lu  DoIP TX: %lu\n",
           (unsigned long)s->can_rx_count,
           (unsigned long)s->rs485_rx_count,
           (unsigned long)s->doip_tx_count);
    pthread_spin_unlock((pthread_spinlock_t *)&s->lock);
}
