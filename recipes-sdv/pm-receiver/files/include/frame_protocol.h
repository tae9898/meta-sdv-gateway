#ifndef FRAME_PROTOCOL_H
#define FRAME_PROTOCOL_H
/*
 * STM32(predictive-maintenance) ↔ RPi3(pm-receiver) UART 프레임 프로토콜 — 수신측 정의.
 * ★ STM32 측 predictive-maintenance/Core/Inc/frame_protocol.h 와 "반드시 동일" (구조체/enum/CRC).
 *    변경 시 양쪽 함께 수정할 것.
 * 프레임: [MAGIC 0xA5][TYPE 1B][LEN 2B LE][PAYLOAD nB][CRC16-CCITT 2B LE]
 *   CRC 범위 = TYPE|LEN|PAYLOAD. poly 0x1021, init 0xFFFF. 리틀엔디언.
 */
#include <stdint.h>
#include <stddef.h>

#define FRAME_MAGIC        0xA5u
#define FRAME_HDR_LEN      4u   /* MAGIC + TYPE + LEN(2) */
#define FRAME_CRC_LEN      2u
#define FRAME_MAX_PAYLOAD  64u

typedef enum {
    FT_SENSOR_RAW     = 0x01,
    FT_FFT_RESULT     = 0x02,
    FT_FEATURE_VECTOR = 0x03,
    FT_ANOMALY_ALERT  = 0x04,
    FT_TEMPERATURE    = 0x05,   /* DS18B20 온도 */
} frame_type_t;

#pragma pack(push, 1)
/* ★ STM32 frame_feature_t 와 동일 (필드 순서/타입 불변) */
typedef struct {
    uint32_t ts_ms;
    int16_t  rms;            /* mg */
    int16_t  peak;           /* mg */
    int16_t  peak2peak;      /* mg */
    int16_t  kurtosis_x100;  /* 첨도 ×100 */
    uint16_t crest_x100;     /* crest ×100 */
    uint16_t f0_x10;         /* Hz ×10 */
    uint32_t band_low;       /* 0-50Hz 에너지 */
    uint32_t band_mid;       /* 50-200Hz */
    uint32_t band_high;      /* 200-500Hz */
    uint8_t  anomaly;        /* 0=정상, 1=이상 */
} frame_feature_t;           /* 29 bytes */

/* ★ STM32 frame_anomaly_t 와 동일 */
typedef struct {
    uint32_t ts_ms;
    int16_t  rms;
    int16_t  kurtosis_x100;
    int16_t  thr_rms;
    int16_t  thr_kurt_x100;
    uint8_t  reason;         /* 0=rms 초과, 1=kurtosis 초과 */
} frame_anomaly_t;           /* 13 bytes */

/* ★ STM32 frame_temperature_t 와 동일 */
typedef struct {
    uint32_t ts_ms;
    int16_t  temp_x100;         /* 온도 °C ×100 */
} frame_temperature_t;         /* 6 bytes */
#pragma pack(pop)

/* CRC16-CCITT (poly 0x1021, init 0xFFFF) — STM32 frame_crc16 과 동일 구현 */
uint16_t frame_crc16(const uint8_t *data, size_t len);

const char *frame_type_name(frame_type_t t);

#endif
