/**
 * @file doip_handler.h
 * @brief DoIP (Diagnostic over IP) send thread
 *
 * Pulls messages from the shared queue, encapsulates them into DoIP frames, and sends over UDP/TCP.
 */

#ifndef DOIP_HANDLER_H
#define DOIP_HANDLER_H

#include <pthread.h>

/**
 * DoIP send thread entry point.
 *
 * @param arg  unused (NULL)
 * @return     NULL
 *
 * Flow:
 *   1. Create UDP socket (port 13400, standard DoIP port)
 *   2. Loop: gw_queue_pop() → build DoIP header → sendto()
 *
 * DoIP frame format (ISO 13400):
 *   [Protocol Version: 1byte][Inverse Version: 1byte]
 *   [Payload Type: 2byte][Payload Length: 4byte][Payload: N bytes]
 *
 * Payload Type used:
 *   0x8001: Diagnostic message (CAN/RS485 → IP)
 */
void *doip_tx_thread(void *arg);

/**
 * DoIP receive thread entry point (bidirectional: Tester → DoIP → CAN).
 * Receive UDP 13400 → parse DoIP diagnostic → send CAN 0x7E0.
 * Remembers the requester address so doip_tx replies with unicast.
 */
void *doip_rx_thread(void *arg);

#endif /* DOIP_HANDLER_H */
