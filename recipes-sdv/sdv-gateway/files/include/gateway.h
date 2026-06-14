/**
 * @file gateway.h
 * @brief SDV Gateway - shared queue, ring buffer, and throughput counter
 *
 * CAN/RS485/DoIP 메시지 라우팅을 위한 공유 데이터 구조.
 * pthread_mutex로 큐 보호, pthread_spinlock로 throughput counter 보호.
 */

#ifndef GATEWAY_H
#define GATEWAY_H

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

/* 메시지 타입 */
typedef enum {
    MSG_TYPE_CAN,       /* SocketCAN 수신 메시지 */
    MSG_TYPE_RS485,     /* RS485 수신 메시지 */
} msg_type_t;

/* 최대 페이로드 크기 (CAN-FD 최대 64바이트 + 오버헤드) */
#define GW_MAX_PAYLOAD  128

/* 게이트웨이 메시지 */
typedef struct {
    msg_type_t type;
    uint32_t   can_id;          /* CAN ID (MSG_TYPE_CAN인 경우) */
    uint8_t    dlc;             /* 데이터 길이 */
    uint8_t    payload[GW_MAX_PAYLOAD];
    uint64_t   timestamp_ns;    /* 수신 시각 (CLOCK_MONOTONIC ns) */
} gw_message_t;

/* Ring buffer 기반 공유 큐 */
#define GW_QUEUE_DEPTH 64

typedef struct {
    gw_message_t    buffer[GW_QUEUE_DEPTH];
    volatile int    head;           /* 읽기 위치 */
    volatile int    tail;           /* 쓰기 위치 */
    pthread_mutex_t mutex;
    pthread_cond_t  cond_not_empty; /* 소비자 대기용 */
} gw_queue_t;

/* Throughput 통계 */
typedef struct {
    atomic_ulong     can_rx_count;
    atomic_ulong     rs485_rx_count;
    atomic_ulong     doip_tx_count;
    pthread_spinlock_t lock;        /* 카운터 읽기 일관성용 */
} gw_stats_t;

/* 초기화 / 해제 */
int  gw_queue_init(gw_queue_t *q);
void gw_queue_destroy(gw_queue_t *q);

/* 생산자: ISR/스레드에서 메시지 enqueue */
int  gw_queue_push(gw_queue_t *q, const gw_message_t *msg);

/* 소비자: DoIP 스레드에서 메시지 dequeue (block) */
int  gw_queue_pop(gw_queue_t *q, gw_message_t *msg);

/* 통계 */
int  gw_stats_init(gw_stats_t *s);
void gw_stats_destroy(gw_stats_t *s);
void gw_stats_print(const gw_stats_t *s);

/* 공용 글로벌 참조 (main.c에서 정의) */
extern gw_queue_t  g_rx_queue;
extern gw_stats_t  g_stats;
extern volatile int g_running;

#endif /* GATEWAY_H */
