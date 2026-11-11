#include "sms_app.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

void *alert_thread(void *arg)
{
    (void)arg;
    sms_message_t msg;

    while (sms_running) {
        struct timespec deadline;

        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += 1;

        ssize_t n = mq_timedreceive(
            sms_alert_mq,
            (char *)&msg,
            sizeof(msg),
            NULL,
            &deadline
        );

        if (n < 0) {
            if (errno == ETIMEDOUT || errno == EINTR || errno == EAGAIN)
                continue;

            fprintf(stderr, "Alert mq_receive: %s\n",
                    strerror(errno));
            break;
        }

        printf("[ALERT] time=%llu temp=%.2f humid=%.2f flags=%d\n",
               (unsigned long long)msg.timestamp_ms,
               msg.temperature,
               msg.humidity,
               msg.alert_flags);
    }

    return NULL;
}
