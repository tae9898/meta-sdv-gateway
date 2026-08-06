#ifndef FRAME_DECODER_H
#define FRAME_DECODER_H
#include "frame_protocol.h"

/* The decoder is fed bytes and, after 0xA5 sync + CRC validation, emits a complete frame. */
typedef struct {
    uint8_t buf[FRAME_HDR_LEN + FRAME_MAX_PAYLOAD + FRAME_CRC_LEN];
    int     pos;     /* number of bytes accumulated in buf */
} frame_decoder_t;

/* Decode result. payload points into the decoder's internal buffer (valid until the next feed) */
typedef struct {
    frame_type_t    type;
    uint16_t        payload_len;
    const uint8_t  *payload;
} frame_t;

void frame_decoder_init(frame_decoder_t *d);

/* Feed one byte. Returns 1 (filling out) when a valid frame completes, else 0 */
int  frame_decoder_feed(frame_decoder_t *d, uint8_t byte, frame_t *out);

#endif
