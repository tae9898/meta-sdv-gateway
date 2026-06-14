/**
 * @file main.c
 * @brief SDV Multi-protocol Diagnostic Gateway - 메인 진입점
 *
 * 3개의 pthread 생성:
 *   1. CAN 수신 (SocketCAN)
 *   2. RS485 수신 (USB-Serial)
 *   3. DoIP 송신 (UDP)
 *
 * 공유 Queue + pthread_mutex로 스레드 간 통신.
 * SIGINT/SIGTERM으로 정상 종료.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gateway.h"
#include "can_handler.h"
#include "rs485_handler.h"
#include "doip_handler.h"

/* 글로벌 상태 */
gw_queue_t   g_rx_queue;
gw_stats_t   g_stats;
volatile int g_running = 1;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
    /* 대기 중인 스레드 깨우기 */
    pthread_mutex_lock(&g_rx_queue.mutex);
    pthread_cond_broadcast(&g_rx_queue.cond_not_empty);
    pthread_mutex_unlock(&g_rx_queue.mutex);
}

static void setup_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int main(void)
{
    pthread_t tid_can, tid_rs485, tid_doip;

    printf("[gateway] SDV Gateway starting...\n");

    /* 공유 리소스 초기화 */
    if (gw_queue_init(&g_rx_queue) != 0) {
        fprintf(stderr, "[gateway] queue init failed\n");
        return 1;
    }
    if (gw_stats_init(&g_stats) != 0) {
        fprintf(stderr, "[gateway] stats init failed\n");
        return 1;
    }

    setup_signals();

    /* 스레드 생성 */
    pthread_create(&tid_can,   NULL, can_rx_thread,   NULL);
    pthread_create(&tid_rs485, NULL, rs485_rx_thread,  NULL);
    pthread_create(&tid_doip,  NULL, doip_tx_thread,   NULL);

    printf("[gateway] 3 threads started. Press Ctrl+C to stop.\n");

    /* 메인 루프: 통계 주기적 출력 */
    while (g_running) {
        sleep(5);
        if (g_running) {
            gw_stats_print(&g_stats);
        }
    }

    printf("\n[gateway] shutting down...\n");

    /* 스레드 종료 대기 */
    pthread_join(tid_doip,  NULL);
    pthread_join(tid_can,   NULL);
    pthread_join(tid_rs485, NULL);

    /* 최종 통계 */
    printf("[gateway] final stats:\n");
    gw_stats_print(&g_stats);

    gw_stats_destroy(&g_stats);
    gw_queue_destroy(&g_rx_queue);

    printf("[gateway] stopped.\n");
    return 0;
}
