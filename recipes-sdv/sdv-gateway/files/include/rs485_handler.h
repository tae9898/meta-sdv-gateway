/**
 * @file rs485_handler.h
 * @brief RS485 수신 스레드 (USB-Serial)
 *
 * USB-RS485 어댑터(/dev/ttyUSB0)로 시리얼 프레임을 수신하여 공유 큐에 push.
 */

#ifndef RS485_HANDLER_H
#define RS485_HANDLER_H

#include <pthread.h>

/**
 * RS485 수신 스레드 진입점.
 *
 * @param arg  사용하지 않음 (NULL)
 * @return     NULL
 *
 * 흐름:
 *   1. open("/dev/ttyUSB0", O_RDWR)
 *   2. termios 설정 (115200 8N1, raw 모드)
 *   3. 루프: read() → 프레임 파싱 [ID_H][ID_L][DLC][DATA] → gw_queue_push()
 *
 * RS485 프레임 포맷 (STM32와 동일):
 *   [ID_H][ID_L][DLC][DATA 0..DLC-1]
 */
void *rs485_rx_thread(void *arg);

#endif /* RS485_HANDLER_H */
