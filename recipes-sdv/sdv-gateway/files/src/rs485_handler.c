/**
 * @file rs485_handler.c
 * @brief RS485 수신 스레드 구현 (USB-Serial)
 *
 * /dev/ttyUSB0 → 프레임 파싱 → gw_queue_push()
 *
 * STM32 RS485 프레임 포맷:
 *   [ID_H][ID_L][DLC][DATA 0..DLC-1]
 */

#include "rs485_handler.h"
#include "gateway.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#define RS485_DEVICE  "/dev/ttyUSB0"
#define RS485_BAUD    B115200

/* 최소 프레임 길이: ID_H + ID_L + DLC = 3바이트 */
#define FRAME_HEADER_LEN  3

static uint64_t get_monotonic_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int configure_serial(int fd)
{
    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd, &tty) != 0) {
        perror("[rs485] tcgetattr failed");
        return -1;
    }

    /* 115200 8N1, raw 모드 */
    cfsetospeed(&tty, RS485_BAUD);
    cfsetispeed(&tty, RS485_BAUD);

    tty.c_cflag &= ~PARENB;        /* no parity */
    tty.c_cflag &= ~CSTOPB;        /* 1 stop bit */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;            /* 8 data bits */
    tty.c_cflag &= ~CRTSCTS;       /* no hardware flow control */
    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);  /* raw input */
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);           /* no software flow control */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;

    /* read()는 최소 1바이트, 타임아웃 100ms */
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("[rs485] tcsetattr failed");
        return -1;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

/**
 * 프레임 파싱 상태머신.
 * [ID_H][ID_L][DLC][DATA 0..DLC-1]
 */
typedef struct {
    uint8_t buf[GW_MAX_PAYLOAD + FRAME_HEADER_LEN];
    int     pos;
    int     frame_len;  /* FRAME_HEADER_LEN + DLC */
    enum { SYNC, DATA } state;
} frame_parser_t;

void *rs485_rx_thread(void *arg)
{
    (void)arg;

    int fd = open(RS485_DEVICE, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        perror("[rs485] open failed (is USB-RS485 connected?)");
        return NULL;
    }

    if (configure_serial(fd) != 0) {
        close(fd);
        return NULL;
    }

    printf("[rs485] listening on %s @ 115200bps\n", RS485_DEVICE);

    frame_parser_t parser;
    memset(&parser, 0, sizeof(parser));

    while (g_running) {
        uint8_t byte;
        ssize_t n = read(fd, &byte, 1);
        if (n <= 0) {
            if (!g_running) break;
            if (errno == EAGAIN || errno == EINTR) continue;
            perror("[rs485] read error");
            continue;
        }

        parser.buf[parser.pos++] = byte;

        if (parser.state == SYNC && parser.pos >= FRAME_HEADER_LEN) {
            parser.frame_len = FRAME_HEADER_LEN + parser.buf[2]; /* buf[2] = DLC */
            if (parser.frame_len > (int)sizeof(parser.buf)) {
                /* 잘못된 프레임 — 리셋 */
                memset(&parser, 0, sizeof(parser));
                continue;
            }
            parser.state = DATA;
        }

        if (parser.state == DATA && parser.pos >= parser.frame_len) {
            /* 완전한 프레임 수신 */
            gw_message_t msg;
            memset(&msg, 0, sizeof(msg));
            msg.type = MSG_TYPE_RS485;
            msg.can_id = ((uint32_t)parser.buf[0] << 8) | parser.buf[1];
            msg.dlc = parser.buf[2];
            msg.timestamp_ns = get_monotonic_ns();

            size_t copy_len = msg.dlc;
            if (copy_len > GW_MAX_PAYLOAD) copy_len = GW_MAX_PAYLOAD;
            memcpy(msg.payload, &parser.buf[FRAME_HEADER_LEN], copy_len);

            gw_queue_push(&g_rx_queue, &msg);
            atomic_fetch_add(&g_stats.rs485_rx_count, 1);

            /* 파서 리셋 */
            memset(&parser, 0, sizeof(parser));
        }
    }

    printf("[rs485] stopping\n");
    close(fd);
    return NULL;
}
