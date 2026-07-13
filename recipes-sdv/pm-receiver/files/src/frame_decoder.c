/* frame_decoder.c — STM32 프레임 디코더 (Phase 0 Python 디코더의 C 포팅)
 * 바이트 스트림 → 0xA5 동기 → CRC16-CCITT 검증 → 프레임 추출.
 * 텍스트([VIB] 등)/CRC 불일치는 건너뛰고 다음 0xA5로 재동기화. */
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

/* 한 바이트 주입. 완전히 유효한 프레임이 완성되면 1 반환 (out 채움). payload는 d->buf 내부 포인터. */
int frame_decoder_feed(frame_decoder_t *d, uint8_t byte, frame_t *out) {
    if (d->pos >= (int)sizeof(d->buf)) {        /* 오버플로 가드 → 리셋 */
        d->pos = 0;
    }
    d->buf[d->pos++] = byte;

    for (;;) {
        /* 1) 버퍼 앞이 MAGIC이 될 때까지 앞부분 drop (텍스트 skip, 빠른 스캔) */
        if (d->pos == 0) return 0;
        if (d->buf[0] != (uint8_t)FRAME_MAGIC) {
            int i = 0;
            while (i < d->pos && d->buf[i] != (uint8_t)FRAME_MAGIC) i++;
            if (i > 0) { memmove(d->buf, d->buf + i, d->pos - i); d->pos -= i; }
            if (d->pos == 0) return 0;
        }
        /* 2) 헤더(4B) 대기 */
        if (d->pos < (int)FRAME_HDR_LEN) return 0;
        uint16_t plen = (uint16_t)(d->buf[2] | (d->buf[3] << 8));
        if (plen > FRAME_MAX_PAYLOAD) {          /* 비정상 길이 → MAGIC 한 바트 버림 */
            memmove(d->buf, d->buf + 1, d->pos - 1); d->pos--; continue;
        }
        /* 3) 전체 프레임 도착 대기 */
        int total = (int)(FRAME_HDR_LEN + plen + FRAME_CRC_LEN);
        if (d->pos < total) return 0;
        /* 4) CRC 검증 (TYPE|LEN|PAYLOAD) */
        uint16_t crc_calc = frame_crc16(d->buf + 1, 3u + plen);
        uint16_t crc_recv = (uint16_t)(d->buf[4 + plen] | (d->buf[4 + plen + 1] << 8));
        if (crc_calc != crc_recv) {              /* CRC 불일치 → 가짜 매직, 재동기화 */
            memmove(d->buf, d->buf + 1, d->pos - 1); d->pos--; continue;
        }
        /* 5) 유효 프레임 */
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
