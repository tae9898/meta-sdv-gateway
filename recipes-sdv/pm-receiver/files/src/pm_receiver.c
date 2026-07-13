/* pm_receiver.c — STM32 UART 프레임 → SQLite 수신기 (Project 4 Phase 1a)
 * 사용: pm-receiver [serial_dev] [db_path]   (기본 /dev/ttyACM0, sensors.db)
 * 직렬 open/termios 패턴은 sdv-gateway rs485_handler.c 차용. */
#include "frame_protocol.h"
#include "frame_decoder.h"
#include "sensor_db.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <time.h>

static volatile sig_atomic_t g_running = 1;
static void on_signal(int sig) { (void)sig; g_running = 0; }

/* 115200 8N1 raw 모드로 직렬 포트 open */
static int open_serial(const char *dev) {
    int fd = open(dev, O_RDWR | O_NOCTTY);
    if (fd < 0) return -1;
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) { close(fd); return -1; }
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag &= ~(PARENB | CSTOPB | CSIZE | CRTSCTS);
    tty.c_cflag |= CS8 | CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN]  = 1;          /* 최소 1바이트 */
    tty.c_cc[VTIME] = 1;          /* 100ms 타임아웃 */
    if (tcsetattr(fd, TCSANOW, &tty) != 0) { close(fd); return -1; }
    tcflush(fd, TCIFLUSH);
    return fd;
}

int main(int argc, char **argv) {
    const char *dev     = (argc > 1) ? argv[1] : "/dev/ttyACM0";
    const char *dbpath  = (argc > 2) ? argv[2] : "sensors.db";

    setvbuf(stdout, NULL, _IOLBF, 0);
    struct sigaction sa; memset(&sa, 0, sizeof sa); sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL); sigaction(SIGTERM, &sa, NULL);

    sensor_db_t db;
    if (sensor_db_open(&db, dbpath) != 0) return 1;

    int fd = open_serial(dev);
    if (fd < 0) { perror("[pm-receiver] open serial"); sensor_db_close(&db); return 1; }
    printf("[pm-receiver] listening %s -> %s  (Ctrl-C to stop)\n", dev, dbpath);

    frame_decoder_t dec; frame_decoder_init(&dec);
    unsigned long ok = 0, feat = 0, anom = 0;
    time_t last_stats = time(NULL);
    uint8_t buf[256];

    while (g_running) {
        ssize_t n = read(fd, buf, sizeof buf);
        if (n <= 0) {
            if (!g_running) break;
            if (errno == EINTR) continue;
            perror("[pm-receiver] read"); sleep(1); continue;
        }
        for (ssize_t i = 0; i < n; i++) {
            frame_t fr;
            if (!frame_decoder_feed(&dec, buf[i], &fr)) continue;
            ok++;
            if (fr.type == FT_FEATURE_VECTOR && fr.payload_len == sizeof(frame_feature_t)) {
                frame_feature_t f; memcpy(&f, fr.payload, sizeof f);
                sensor_db_insert_feature(&db, &f); feat++;
                printf("[FEATURE] ts=%u rms=%d kurt=%.2f f0=%.1f anomaly=%u\n",
                       (unsigned)f.ts_ms, (int)f.rms, f.kurtosis_x100 / 100.0,
                       f.f0_x10 / 10.0, (unsigned)f.anomaly);
            } else if (fr.type == FT_ANOMALY_ALERT && fr.payload_len == sizeof(frame_anomaly_t)) {
                frame_anomaly_t a; memcpy(&a, fr.payload, sizeof a);
                sensor_db_insert_anomaly(&db, &a); anom++;
                printf("[ANOMALY] ts=%u rms=%d(thr %d) kurt=%.2f(thr %.2f) reason=%u\n",
                       (unsigned)a.ts_ms, (int)a.rms, (int)a.thr_rms,
                       a.kurtosis_x100 / 100.0, a.thr_kurt_x100 / 100.0, (unsigned)a.reason);
            } else if (fr.type == FT_TEMPERATURE && fr.payload_len == sizeof(frame_temperature_t)) {
                frame_temperature_t t; memcpy(&t, fr.payload, sizeof t);
                sensor_db_insert_temperature(&db, &t);
                printf("[TEMP] ts=%u temp=%.2fC\n", (unsigned)t.ts_ms, t.temp_x100 / 100.0);
            }
        }
        time_t now = time(NULL);
        if (now - last_stats >= 5) {
            last_stats = now;
            printf("[stats] ok=%lu feature=%lu anomaly=%lu\n", ok, feat, anom);
        }
    }

    printf("\n[pm-receiver] stopping. ok=%lu feature=%lu anomaly=%lu\n", ok, feat, anom);
    close(fd);
    sensor_db_close(&db);
    return 0;
}
