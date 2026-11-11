#ifndef SMS_APP_H
#define SMS_APP_H

#include <pthread.h>
#include <mqueue.h>
#include <signal.h>
#include <stdint.h>

#define SMS_BUFFER_SIZE 128
#define SMS_AVG_WINDOW 10
#define SMS_LOG_PATH "/tmp/sms.log"
#define SMS_LOG_MAX_SIZE (1024 * 1024)

typedef struct {
    uint64_t timestamp_ms;
    double temperature;
    double humidity;
} sensor_sample_t;

typedef struct {
    sensor_sample_t data[SMS_BUFFER_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} sensor_buffer_t;

typedef struct {
    unsigned int interval_ms;
    double temp_threshold;
    double humid_threshold;
    unsigned int duration_sec;
    int verbose;
} sms_config_t;

#define SMS_ALERT_QUEUE "/sms_alert"
#define SMS_LOG_QUEUE "/sms_log"

typedef struct {
    uint64_t timestamp_ms;
    double temperature;
    double humidity;
    double avg_temperature;
    double avg_humidity;
    int alert_flags;
} sms_message_t;

#define SMS_ALERT_TEMP  1
#define SMS_ALERT_HUMID 2
#define SMS_ALERT_SPIKE 4

extern mqd_t sms_alert_mq;
extern mqd_t sms_log_mq;

extern volatile sig_atomic_t sms_running;
extern sensor_buffer_t sms_buffer;
extern sms_config_t sms_config;

void *sensor_thread(void *arg);
void *processor_thread(void *arg);
void *alert_thread(void *arg);
void *logger_thread(void *arg);

#endif
