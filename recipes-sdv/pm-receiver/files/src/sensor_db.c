/* sensor_db.c — SQLite 시계열 저장 (feature_vector / anomaly_event) */
#include "sensor_db.h"
#include <stdio.h>
#include <string.h>

#define SCHEMA_SQL \
    "PRAGMA journal_mode=WAL;" \
    "PRAGMA synchronous=NORMAL;" \
    "CREATE TABLE IF NOT EXISTS feature_vector(" \
    "  ts INTEGER DEFAULT (strftime('%s','now'))," \
    "  ts_src INTEGER, rms REAL, peak REAL, p2p REAL," \
    "  kurtosis REAL, crest REAL, f0 REAL," \
    "  band_low REAL, band_mid REAL, band_high REAL," \
    "  anomaly INTEGER);" \
    "CREATE INDEX IF NOT EXISTS idx_fv_ts ON feature_vector(ts DESC);" \
    "CREATE TABLE IF NOT EXISTS anomaly_event(" \
    "  ts INTEGER DEFAULT (strftime('%s','now'))," \
    "  ts_src INTEGER, rms REAL, kurt REAL, thr_rms REAL, thr_kurt REAL, reason INTEGER);" \
    "CREATE INDEX IF NOT EXISTS idx_ae_ts ON anomaly_event(ts DESC);"

#define INS_FEATURE \
    "INSERT INTO feature_vector(ts_src,rms,peak,p2p,kurtosis,crest,f0,band_low,band_mid,band_high,anomaly)" \
    " VALUES (?,?,?,?,?,?,?,?,?,?,?);"
#define INS_ANOMALY \
    "INSERT INTO anomaly_event(ts_src,rms,kurt,thr_rms,thr_kurt,reason) VALUES (?,?,?,?,?,?);"

int sensor_db_open(sensor_db_t *s, const char *path) {
    memset(s, 0, sizeof(*s));
    if (sqlite3_open(path, &s->db) != SQLITE_OK) {
        fprintf(stderr, "[db] open %s: %s\n", path, sqlite3_errmsg(s->db));
        return -1;
    }
    if (sensor_db_create_schema(s) != 0) return -1;
    if (sqlite3_prepare_v2(s->db, INS_FEATURE, -1, &s->stmt_feature, NULL) != SQLITE_OK ||
        sqlite3_prepare_v2(s->db, INS_ANOMALY, -1, &s->stmt_anomaly, NULL) != SQLITE_OK) {
        fprintf(stderr, "[db] prepare: %s\n", sqlite3_errmsg(s->db));
        return -1;
    }
    printf("[db] opened %s (WAL)\n", path);
    return 0;
}

int sensor_db_create_schema(sensor_db_t *s) {
    char *err = NULL;
    if (sqlite3_exec(s->db, SCHEMA_SQL, NULL, NULL, &err) != SQLITE_OK) {
        fprintf(stderr, "[db] schema: %s\n", err ? err : "?");
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

int sensor_db_insert_feature(sensor_db_t *s, const frame_feature_t *f) {
    sqlite3_stmt *st = s->stmt_feature;
    sqlite3_bind_int64(st, 1,  (sqlite3_int64)f->ts_ms);
    sqlite3_bind_double(st, 2, f->rms);
    sqlite3_bind_double(st, 3, f->peak);
    sqlite3_bind_double(st, 4, f->peak2peak);
    sqlite3_bind_double(st, 5, (double)f->kurtosis_x100 / 100.0);
    sqlite3_bind_double(st, 6, (double)f->crest_x100 / 100.0);
    sqlite3_bind_double(st, 7, (double)f->f0_x10 / 10.0);
    sqlite3_bind_int64(st, 8,  (sqlite3_int64)f->band_low);
    sqlite3_bind_int64(st, 9,  (sqlite3_int64)f->band_mid);
    sqlite3_bind_int64(st, 10, (sqlite3_int64)f->band_high);
    sqlite3_bind_int(st, 11,   f->anomaly);
    int rc = sqlite3_step(st);
    sqlite3_reset(st);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int sensor_db_insert_anomaly(sensor_db_t *s, const frame_anomaly_t *a) {
    sqlite3_stmt *st = s->stmt_anomaly;
    sqlite3_bind_int64(st, 1, (sqlite3_int64)a->ts_ms);
    sqlite3_bind_double(st, 2, a->rms);
    sqlite3_bind_double(st, 3, (double)a->kurtosis_x100 / 100.0);
    sqlite3_bind_double(st, 4, a->thr_rms);
    sqlite3_bind_double(st, 5, (double)a->thr_kurt_x100 / 100.0);
    sqlite3_bind_int(st, 6, a->reason);
    int rc = sqlite3_step(st);
    sqlite3_reset(st);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

void sensor_db_close(sensor_db_t *s) {
    if (s->stmt_feature) sqlite3_finalize(s->stmt_feature);
    if (s->stmt_anomaly) sqlite3_finalize(s->stmt_anomaly);
    if (s->db) sqlite3_close(s->db);
    memset(s, 0, sizeof(*s));
}
