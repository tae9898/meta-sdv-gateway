/* frame_decoder.c — STM32 frame decoder (C port of the Phase 0 Python decoder)
 * Byte stream → 0xA5 sync → CRC16-CCITT validation → frame extraction.
 * Skips text ([VIB] etc.)/CRC mismatches and re-syncs to the next 0xA5. */
#include "frame_decoder.h"
#include <string.h>

uint16_t frame_crc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    }
    return crc;
}

const char *frame_type_name(frame_type_t t) {
    switch (t) {
        case FT_SENSOR_RAW:     return "SENSOR_RAW";
        case FT_FFT_RESULT:     return "FFT_RESULT";
        case FT_FEATURE_VECTOR: return "FEATURE_VECTOR";
        case FT_ANOMALY_ALERT:  return "ANOMALY_ALERT";
        case FT_TEMPERATURE:    return "TEMPERATURE";
        default:                return "UNKNOWN";
    }
}

void frame_decoder_init(frame_decoder_t *d) {
    memset(d, 0, sizeof(*d));
}

/* Feed one byte. Returns 1 when a fully valid frame completes (filling out). payload points into d->buf. */
int frame_decoder_feed(frame_decoder_t *d, uint8_t byte, frame_t *out) {
    if (d->pos >= (int)sizeof(d->buf)) {        /* overflow guard → reset */
        d->pos = 0;
    }
    d->buf[d->pos++] = byte;

    for (;;) {
        /* 1) Drop bytes from the front until the buffer starts with MAGIC (skip text, fast scan) */
        if (d->pos == 0) return 0;
        if (d->buf[0] != (uint8_t)FRAME_MAGIC) {
            int i = 0;
            while (i < d->pos && d->buf[i] != (uint8_t)FRAME_MAGIC) i++;
            if (i > 0) { memmove(d->buf, d->buf + i, d->pos - i); d->pos -= i; }
            if (d->pos == 0) return 0;
        }
        /* 2) Wait for header (4B) */
        if (d->pos < (int)FRAME_HDR_LEN) return 0;
        uint16_t plen = (uint16_t)(d->buf[2] | (d->buf[3] << 8));
        if (plen > FRAME_MAX_PAYLOAD) {          /* abnormal length → discard one MAGIC byte */
            memmove(d->buf, d->buf + 1, d->pos - 1); d->pos--; continue;
        }
        /* 3) Wait for the full frame to arrive */
        int total = (int)(FRAME_HDR_LEN + plen + FRAME_CRC_LEN);
        if (d->pos < total) return 0;
        /* 4) CRC validation (TYPE|LEN|PAYLOAD) */
        uint16_t crc_calc = frame_crc16(d->buf + 1, 3u + plen);
        uint16_t crc_recv = (uint16_t)(d->buf[4 + plen] | (d->buf[4 + plen + 1] << 8));
        if (crc_calc != crc_recv) {              /* CRC mismatch → fake magic, re-sync */
            memmove(d->buf, d->buf + 1, d->pos - 1); d->pos--; continue;
        }
        /* 5) Valid frame */
        if (out) {
            out->type = (frame_type_t)d->buf[1];
            out->payload_len = plen;
            out->payload = d->buf + FRAME_HDR_LEN;
        }
        int remain = d->pos - total;
        if (remain > 0) memmove(d->buf, d->buf + total, remain);
        d->pos = remain;
        return 1;
    }
}
