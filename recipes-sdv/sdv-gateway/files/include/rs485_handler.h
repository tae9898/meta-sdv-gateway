/**
 * @file rs485_handler.h
 * @brief RS485 receive thread (USB-Serial)
 *
 * Receives serial frames from the USB-RS485 adapter (/dev/ttyUSB0) and pushes them to the shared queue.
 */

#ifndef RS485_HANDLER_H
#define RS485_HANDLER_H

#include <pthread.h>

/**
 * RS485 receive thread entry point.
 *
 * @param arg  unused (NULL)
 * @return     NULL
 *
 * Flow:
 *   1. open("/dev/ttyUSB0", O_RDWR)
 *   2. termios setup (115200 8N1, raw mode)
 *   3. Loop: read() → parse frame [ID_H][ID_L][DLC][DATA] → gw_queue_push()
 *
 * RS485 frame format (same as STM32):
 *   [ID_H][ID_L][DLC][DATA 0..DLC-1]
 */
void *rs485_rx_thread(void *arg);

#endif /* RS485_HANDLER_H */
