/**
 * @file doip_handler.c
 * @brief DoIP 송신 스레드 구현
 *
 * 공유 큐에서 메시지를 pull → DoIP 프레임 캡슐화 → UDP 전송.
 *
 * DoIP 헤더 포맷 (ISO 13400, 2019):
 *   [Protocol Version: 1B][Inverse Version: 1B]
 *   [Payload Type: 2B][Payload Length: 4B]
 *   [Payload: N bytes]
 *
 * Payload Type 0x8001: Vehicle Identification Request / Diagnostic Message
 */

#include "doip_handler.h"
#include "gateway.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DOIP_PORT       13400
#define DOIP_VERSION    0x02
#define DOIP_INV_VER    0xFD    /* ~0x02 */

/* Diagnostic Message (payload type 0x8001) */
#define DOIP_PT_DIAG_MSG  0x8001

/* DoIP 헤더 크기 */
#define DOIP_HEADER_LEN   8

/**
 * DoIP 헤더 생성.
 *
 * @param out        출력 버퍼 (최소 DOIP_HEADER_LEN + payload_len 크기)
 * @param ptype      Payload Type
 * @param payload    페이로드 데이터
 * @param plen       페이로드 길이
 * @return           전체 프레임 길이
 */
static size_t build_doip_frame(uint8_t *out, uint16_t ptype,
                                const uint8_t *payload, uint16_t plen)
{
    out[0] = DOIP_VERSION;
    out[1] = DOIP_INV_VER;
    out[2] = (ptype >> 8) & 0xFF;
    out[3] = ptype & 0xFF;
    out[4] = (plen >> 24) & 0xFF;
    out[5] = (plen >> 16) & 0xFF;
    out[6] = (plen >> 8) & 0xFF;
    out[7] = plen & 0xFF;

    if (plen > 0 && payload != NULL) {
        memcpy(&out[DOIP_HEADER_LEN], payload, plen);
    }

    return DOIP_HEADER_LEN + plen;
}

/**
 * gw_message_t → DoIP diagnostic payload 변환.
 *
 * Diagnostic payload 포맷:
 *   [Source Address: 2B][Target Address: 2B][Data: N bytes]
 */
static uint16_t msg_to_diag_payload(const gw_message_t *msg, uint8_t *out,
                                     size_t out_size)
{
    /* Source: 게이트웨이 (0x0E00), Target: Tester (0x0E00) */
    uint16_t src_addr = 0x0E00;
    uint16_t tgt_addr = 0x0E00;

    size_t needed = 4 + msg->dlc;
    if (needed > out_size) {
        /* 페이로드 잘림 */
        needed = out_size;
    }

    out[0] = (src_addr >> 8) & 0xFF;
    out[1] = src_addr & 0xFF;
    out[2] = (tgt_addr >> 8) & 0xFF;
    out[3] = tgt_addr & 0xFF;

    size_t data_len = needed - 4;
    if (data_len > 0) {
        memcpy(&out[4], msg->payload, data_len);
    }

    return (uint16_t)needed;
}

void *doip_tx_thread(void *arg)
{
    (void)arg;

    /* UDP 소켓 */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("[doip] socket failed");
        return NULL;
    }

    /* 브로드스트 enables */
    int broadcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    /* 타겟 주소: 브로드스트로 전송 (포팠폴리오 데모용) */
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(DOIP_PORT);
    dest.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    printf("[doip] sending to UDP port %d (broadcast)\n", DOIP_PORT);

    /* 송신 버퍼 */
    uint8_t diag_buf[GW_MAX_PAYLOAD + 4];    /* addr(4) + payload */
    uint8_t doip_buf[DOIP_HEADER_LEN + sizeof(diag_buf)];

    while (g_running) {
        gw_message_t msg;
        if (gw_queue_pop(&g_rx_queue, &msg) != 0) {
            break;  /* 종료 신호 */
        }

        /* 메시지 → DoIP diagnostic payload → DoIP 프레임 */
        uint16_t diag_len = msg_to_diag_payload(&msg, diag_buf, sizeof(diag_buf));
        size_t frame_len = build_doip_frame(doip_buf, DOIP_PT_DIAG_MSG,
                                            diag_buf, diag_len);

        ssize_t sent = sendto(sock, doip_buf, frame_len, 0,
                              (struct sockaddr *)&dest, sizeof(dest));
        if (sent < 0) {
            perror("[doip] sendto failed");
        } else {
            atomic_fetch_add(&g_stats.doip_tx_count, 1);
        }
    }

    printf("[doip] stopping\n");
    close(sock);
    return NULL;
}
