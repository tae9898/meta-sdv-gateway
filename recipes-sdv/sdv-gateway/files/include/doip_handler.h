/**
 * @file doip_handler.h
 * @brief DoIP (Diagnostic over IP) 송신 스레드
 *
 * 공유 큐에서 메시지를 pull하여 DoIP 프레임으로 캡슐화 후 UDP/TCP 전송.
 */

#ifndef DOIP_HANDLER_H
#define DOIP_HANDLER_H

#include <pthread.h>

/**
 * DoIP 송신 스레드 진입점.
 *
 * @param arg  사용하지 않음 (NULL)
 * @return     NULL
 *
 * 흐름:
 *   1. UDP 소켓 생성 (포트 13400, DoIP 표준 포트)
 *   2. 루프: gw_queue_pop() → DoIP 헤더 생성 → sendto()
 *
 * DoIP 프레임 포맷 (ISO 13400):
 *   [Protocol Version: 1byte][Inverse Version: 1byte]
 *   [Payload Type: 2byte][Payload Length: 4byte][Payload: N bytes]
 *
 * 사용하는 Payload Type:
 *   0x8001: Diagnostic message (CAN/RS485 → IP)
 */
void *doip_tx_thread(void *arg);

#endif /* DOIP_HANDLER_H */
