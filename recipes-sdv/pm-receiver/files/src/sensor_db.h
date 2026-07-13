#ifndef SENSOR_DB_H
#define SENSOR_DB_H
#include "frame_protocol.h"
#include <sqlite3.h>

/* SQLite 시계열 저장. 열린 DB는 단일 스레드(pm-receiver main)에서만 사용. */
typedef struct {
    sqlite3 *db;
    sqlite3_stmt *stmt_feature;
    sqlite3_stmt *stmt_anomaly;
    sqlite3_stmt *stmt_temp;
} sensor_db_t;

int  sensor_db_open(sensor_db_t *s, const char *path);
int  sensor_db_create_schema(sensor_db_t *s);
int  sensor_db_insert_feature(sensor_db_t *s, const frame_feature_t *f);
int  sensor_db_insert_anomaly(sensor_db_t *s, const frame_anomaly_t *a);
int  sensor_db_insert_temperature(sensor_db_t *s, const frame_temperature_t *t);
void sensor_db_close(sensor_db_t *s);

#endif
