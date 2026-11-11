#include "sms_app.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

void *logger_thread(void *arg)
{
    (void)arg;
    sms_message_t msg;
    FILE *fp = fopen(SMS_LOG_PATH, "a");

    if (!fp) {
        perror("fopen sms.log");
        return NULL;
    }

    time_t last_flush = time(NULL);

    while (sms_running) {
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += 1;

        ssize_t n = mq_timedreceive(
            sms_log_mq, (char *)&msg, sizeof(msg),
            NULL, &deadline
        );

        if (n >= 0) {
            fprintf(fp, "%llu,%.2f,%.2f,%.2f,%.2f,%d\n",
                    (unsigned long long)msg.timestamp_ms,
                    msg.temperature, msg.humidity,
                    msg.avg_temperature, msg.avg_humidity,
                    msg.alert_flags);
        } else if (errno != ETIMEDOUT && errno != EINTR && errno != EAGAIN) {
            perror("logger mq_timedreceive");
            break;
        }

        if (time(NULL) - last_flush >= 5) {
            fflush(fp);
            last_flush = time(NULL);
        }

        long size = ftell(fp);

        if (size > SMS_LOG_MAX_SIZE) {
            fflush(fp);
            fclose(fp);

            rename(SMS_LOG_PATH, "/tmp/sms.log.1");
            fp = fopen(SMS_LOG_PATH, "w");

            if (!fp) {
                perror("rotate sms.log");
                return NULL;
            }
        }
    }

    fflush(fp);
    fclose(fp);
    return NULL;
}
