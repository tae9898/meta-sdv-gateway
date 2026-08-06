#ifndef FRAME_PROTOCOL_H
#define FRAME_PROTOCOL_H
/*
 * STM32(predictive-maintenance) ↔ RPi3(pm-receiver) UART frame protocol — receiver-side definition.
 * ★ MUST be identical to STM32-side predictive-maintenance/Core/Inc/frame_protocol.h (struct/enum/CRC).
 *    When changing, update both sides together.
 * Frame: [MAGIC 0xA5][TYPE 1B][LEN 2B LE][PAYLOAD nB][CRC16-CCITT 2B LE]
 *   CRC range = TYPE|LEN|PAYLOAD. poly 0x1021, init 0xFFFF. Little-endian.
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
    FT_TEMPERATURE    = 0x05,   /* DS18B20 temperature */
} frame_type_t;

#pragma pack(push, 1)
/* ★ Identical to STM32 frame_feature_t (field order/type immutable) */
typedef struct {
    uint32_t ts_ms;
    int16_t  rms;            /* mg */
    int16_t  peak;           /* mg */
    int16_t  peak2peak;      /* mg */
    int16_t  kurtosis_x100;  /* kurtosis ×100 */
    uint16_t crest_x100;     /* crest ×100 */
    uint16_t f0_x10;         /* Hz ×10 */
    uint32_t band_low;       /* 0-50Hz energy */
    uint32_t band_mid;       /* 50-200Hz */
    uint32_t band_high;      /* 200-500Hz */
    uint8_t  anomaly;        /* 0=normal, 1=anomaly */
} frame_feature_t;           /* 29 bytes */

/* ★ Identical to STM32 frame_anomaly_t */
typedef struct {
    uint32_t ts_ms;
    int16_t  rms;
    int16_t  kurtosis_x100;
    int16_t  thr_rms;
    int16_t  thr_kurt_x100;
    uint8_t  reason;         /* 0=rms exceeded, 1=kurtosis exceeded */
} frame_anomaly_t;           /* 13 bytes */

/* ★ Identical to STM32 frame_temperature_t */
typedef struct {
    uint32_t ts_ms;
    int16_t  temp_x100;         /* temperature °C ×100 */
} frame_temperature_t;         /* 6 bytes */
#pragma pack(pop)

/* CRC16-CCITT (poly 0x1021, init 0xFFFF) — same implementation as STM32 frame_crc16 */
uint16_t frame_crc16(const uint8_t *data, size_t len);

const char *frame_type_name(frame_type_t t);

#endif
