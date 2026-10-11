#include "logger.h"

#include <pthread.h>
#include <stdio.h>
#include <time.h>

static FILE *log_fp;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int logger_init(const char *path) {
    log_fp = fopen(path, "a");
    return log_fp ? 0 : -1;
}

static void timestamp_now(char *buf, size_t n) {
    time_t now = time(NULL);
    struct tm tmv;
    localtime_r(&now, &tmv);
    strftime(buf, n, "%Y-%m-%d %H:%M:%S", &tmv);
}

void logger_log(const char *client_ip, const char *method, const char *url,
                const char *result, int status, size_t bytes, long latency_ms) {
    pthread_mutex_lock(&log_mutex);
    if (log_fp) {
        char ts[32];
        timestamp_now(ts, sizeof(ts));
        fprintf(log_fp, "[%s] %s %s %s %s %d %zu bytes %ldms\n",
                ts, client_ip, method, url, result, status, bytes, latency_ms);
        fflush(log_fp);
    }
    pthread_mutex_unlock(&log_mutex);
}

void logger_log_error(const char *client_ip, const char *message) {
    pthread_mutex_lock(&log_mutex);
    if (log_fp) {
        char ts[32];
        timestamp_now(ts, sizeof(ts));
        fprintf(log_fp, "[%s] %s ERROR %s\n", ts, client_ip, message);
        fflush(log_fp);
    }
    pthread_mutex_unlock(&log_mutex);
}

void logger_close(void) {
    pthread_mutex_lock(&log_mutex);
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
}
