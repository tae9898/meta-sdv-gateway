/**
 * @file gateway.h
 * @brief SDV Gateway - shared queue, ring buffer, and throughput counter
 *
 * Shared data structure for CAN/RS485/DoIP message routing.
 * pthread_mutex protects the queue; pthread_spinlock protects the throughput counter.
 */

#ifndef GATEWAY_H
#define GATEWAY_H

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

/* Message type */
typedef enum {
    MSG_TYPE_CAN,       /* SocketCAN received message */
    MSG_TYPE_RS485,     /* RS485 received message */
} msg_type_t;

/* Max payload size (CAN-FD max 64 bytes + overhead) */
#define GW_MAX_PAYLOAD  128

/* Gateway message */
typedef struct {
    msg_type_t type;
    uint32_t   can_id;          /* CAN ID (when MSG_TYPE_CAN) */
    uint8_t    dlc;             /* data length */
    uint8_t    payload[GW_MAX_PAYLOAD];
    uint64_t   timestamp_ns;    /* receive timestamp (CLOCK_MONOTONIC ns) */
} gw_message_t;

/* Ring-buffer based shared queue */
#define GW_QUEUE_DEPTH 64

typedef struct {
    gw_message_t    buffer[GW_QUEUE_DEPTH];
    volatile int    head;           /* read position */
    volatile int    tail;           /* write position */
    pthread_mutex_t mutex;
    pthread_cond_t  cond_not_empty; /* for consumer to wait */
} gw_queue_t;

/* Throughput statistics */
typedef struct {
    atomic_ulong     can_rx_count;
    atomic_ulong     rs485_rx_count;
    atomic_ulong     doip_tx_count;
    pthread_spinlock_t lock;        /* for counter read consistency */
} gw_stats_t;

/* init / teardown */
int  gw_queue_init(gw_queue_t *q);
void gw_queue_destroy(gw_queue_t *q);

/* Producer: enqueue message from ISR/thread */
int  gw_queue_push(gw_queue_t *q, const gw_message_t *msg);

/* Consumer: dequeue message from DoIP thread (block) */
int  gw_queue_pop(gw_queue_t *q, gw_message_t *msg);

/* Statistics */
int  gw_stats_init(gw_stats_t *s);
void gw_stats_destroy(gw_stats_t *s);
void gw_stats_print(gw_stats_t *s);

/* Utilities (shared across handlers) */
uint64_t gw_get_monotonic_ns(void);                               /* CLOCK_MONOTONIC ns */
int      gw_open_can_socket(const char *ifname, const char *tag); /* bound CAN-FD raw socket, -1 on error (logged by tag) */

/* Shared global references (defined in main.c) */
extern gw_queue_t  g_rx_queue;
extern gw_stats_t  g_stats;
extern volatile int g_running;

#endif /* GATEWAY_H */
