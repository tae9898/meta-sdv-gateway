/**
 * @file can_handler.h
 * @brief SocketCAN 수신 스레드
 *
 * USB-CAN-FD 어댑터로 CAN 프레임을 수신하여 공유 큐에 push.
 */

#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include <pthread.h>

/**
 * SocketCAN 수신 스레드 진입점.
 *
 * @param arg  사용하지 않음 (NULL)
 * @return     NULL
 *
 * 흐름:
 *   1. socket(PF_CAN, SOCK_RAW, CAN_RAW) 열기
 *   2. CAN-FD 활성화 setsockopt(SOL_CAN_RAW, CAN_RAW_FD_FRAMES)
 *   3. 인터페이스 "can0" 바인드
 *   4. 루프: read() → gw_message_t 변환 → gw_queue_push()
 */
void *can_rx_thread(void *arg);

#endif /* CAN_HANDLER_H */
