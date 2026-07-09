#ifndef FRAME_DECODER_H
#define FRAME_DECODER_H
#include "frame_protocol.h"

/* 디코더는 바이트를 주입(feed)하며, 0xA5 동기 + CRC 검증을 거쳐 완전한 프레임을 내놓는다. */
typedef struct {
    uint8_t buf[FRAME_HDR_LEN + FRAME_MAX_PAYLOAD + FRAME_CRC_LEN];
    int     pos;     /* buf에 쌓인 바이트 수 */
} frame_decoder_t;

/* 디코딩 결과. payload는 디코더 내부 버퍼 포인터(다음 feed 전까지 유효) */
typedef struct {
    frame_type_t    type;
    uint16_t        payload_len;
    const uint8_t  *payload;
} frame_t;

void frame_decoder_init(frame_decoder_t *d);

/* 한 바이트 주입. 유효 프레임 완성 시 1 반환(out 채움), 아니면 0 */
int  frame_decoder_feed(frame_decoder_t *d, uint8_t byte, frame_t *out);

#endif
