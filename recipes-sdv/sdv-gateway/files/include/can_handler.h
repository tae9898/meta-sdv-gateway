/**
 * @file can_handler.h
 * @brief SocketCAN receive thread
 *
 * Receives CAN frames from the USB-CAN-FD adapter and pushes them to the shared queue.
 */

#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include <pthread.h>

/**
 * SocketCAN receive thread entry point.
 *
 * @param arg  unused (NULL)
 * @return     NULL
 *
 * Flow:
 *   1. Open socket(PF_CAN, SOCK_RAW, CAN_RAW)
 *   2. Enable CAN-FD with setsockopt(SOL_CAN_RAW, CAN_RAW_FD_FRAMES)
 *   3. Bind interface "can0"
 *   4. Loop: read() → convert to gw_message_t → gw_queue_push()
 */
void *can_rx_thread(void *arg);

#endif /* CAN_HANDLER_H */
