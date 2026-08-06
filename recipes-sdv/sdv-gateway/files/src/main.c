/**
 * @file main.c
 * @brief SDV Multi-protocol Diagnostic Gateway - main entry point
 *
 * Spawns 3 pthreads:
 *   1. CAN receive (SocketCAN)
 *   2. RS485 receive (USB-Serial)
 *   3. DoIP send (UDP)
 *
 * Inter-thread communication via shared Queue + pthread_mutex.
 * Clean shutdown on SIGINT/SIGTERM.
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gateway.h"
#include "can_handler.h"
#include "rs485_handler.h"
#include "doip_handler.h"

/* Global state */
gw_queue_t   g_rx_queue;
gw_stats_t   g_stats;
volatile int g_running = 1;

static void signal_handler(int sig)
{
    (void)sig;
    g_running = 0;
    /* Wake waiting threads */
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
    pthread_t tid_can, tid_rs485, tid_doip, tid_doip_rx;

    /* stdout line-buffer (immediate output to systemd journal) */
    setvbuf(stdout, NULL, _IOLBF, 0);

    printf("[gateway] SDV Gateway starting...\n");

    /* Initialize shared resources */
    if (gw_queue_init(&g_rx_queue) != 0) {
        fprintf(stderr, "[gateway] queue init failed\n");
        return 1;
    }
    if (gw_stats_init(&g_stats) != 0) {
        fprintf(stderr, "[gateway] stats init failed\n");
        return 1;
    }

    setup_signals();

    /* Create threads */
    pthread_create(&tid_can,      NULL, can_rx_thread,   NULL);
    pthread_create(&tid_rs485,    NULL, rs485_rx_thread,  NULL);
    pthread_create(&tid_doip,     NULL, doip_tx_thread,   NULL);
    pthread_create(&tid_doip_rx,  NULL, doip_rx_thread,   NULL);

    printf("[gateway] 4 threads started. Press Ctrl+C to stop.\n");

    /* Main loop: periodic statistics output */
    while (g_running) {
        sleep(5);
        if (g_running) {
            gw_stats_print(&g_stats);
        }
    }

    printf("\n[gateway] shutting down...\n");

    /* Wait for threads to exit */
    pthread_join(tid_doip_rx, NULL);
    pthread_join(tid_doip,    NULL);
    pthread_join(tid_can,     NULL);
    pthread_join(tid_rs485,   NULL);

    /* Final statistics */
    printf("[gateway] final stats:\n");
    gw_stats_print(&g_stats);

    gw_stats_destroy(&g_stats);
    gw_queue_destroy(&g_rx_queue);

    printf("[gateway] stopped.\n");
    return 0;
}
