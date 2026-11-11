#include "sms_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>

volatile sig_atomic_t sms_running = 1;
sensor_buffer_t sms_buffer;
mqd_t sms_alert_mq;
mqd_t sms_log_mq;
sms_config_t sms_config = {
    .interval_ms = 100,
    .temp_threshold = 40.0,
    .humid_threshold = 80.0,
    .duration_sec = 60,
    .verbose = 0
};

static void print_usage(const char *program)
{
    printf("Usage: %s [-i ms] [-t temp] [-h humidity] [-d sec] [-v]\n",
           program);
}

int main(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "i:t:h:d:v")) != -1) {
        switch (opt) {
        case 'i':
            sms_config.interval_ms = (unsigned int)strtoul(optarg, NULL, 10);
            break;
        case 't':
            sms_config.temp_threshold = strtod(optarg, NULL);
            break;
        case 'h':
            sms_config.humid_threshold = strtod(optarg, NULL);
            break;
        case 'd':
            sms_config.duration_sec = (unsigned int)strtoul(optarg, NULL, 10);
            break;
        case 'v':
            sms_config.verbose = 1;
            break;
        default:
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    printf("SMS Configuration:\n");
    printf("Interval: %u ms\n", sms_config.interval_ms);
    printf("Temperature threshold: %.2f C\n", sms_config.temp_threshold);
    printf("Humidity threshold: %.2f %%\n", sms_config.humid_threshold);
    printf("Duration: %u seconds\n", sms_config.duration_sec);
    printf("Verbose: %s\n", sms_config.verbose ? "ON" : "OFF");

    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = sizeof(sms_message_t),
        .mq_curmsgs = 0
    };

    pthread_mutex_init(&sms_buffer.mutex, NULL);
    pthread_cond_init(&sms_buffer.not_empty, NULL);
    pthread_cond_init(&sms_buffer.not_full, NULL);

    mq_unlink(SMS_ALERT_QUEUE);
    mq_unlink(SMS_LOG_QUEUE);

    sms_alert_mq = mq_open(SMS_ALERT_QUEUE,
                           O_CREAT | O_RDWR | O_NONBLOCK,
                           0600, &attr);
    sms_log_mq = mq_open(SMS_LOG_QUEUE,
                         O_CREAT | O_RDWR | O_NONBLOCK,
                         0600, &attr);

    if (sms_alert_mq == (mqd_t)-1 ||
        sms_log_mq == (mqd_t)-1) {
        perror("mq_open");
        return EXIT_FAILURE;
    }

    pthread_t threads[4];
    void *(*workers[4])(void *) = {
        sensor_thread, processor_thread,
        alert_thread, logger_thread
    };

    for (int i = 0; i < 4; i++) {
        int ret = pthread_create(&threads[i], NULL,
                                 workers[i], NULL);
        if (ret != 0) {
            fprintf(stderr, "pthread_create: %s\\n",
                    strerror(ret));
            sms_running = 0;
            return EXIT_FAILURE;
        }
    }

    sleep(sms_config.duration_sec);
    sms_running = 0;

    pthread_mutex_lock(&sms_buffer.mutex);
    pthread_cond_broadcast(&sms_buffer.not_empty);
    pthread_cond_broadcast(&sms_buffer.not_full);
    pthread_mutex_unlock(&sms_buffer.mutex);

    for (int i = 0; i < 4; i++)
        pthread_join(threads[i], NULL);

    mq_close(sms_alert_mq);
    mq_close(sms_log_mq);
    mq_unlink(SMS_ALERT_QUEUE);
    mq_unlink(SMS_LOG_QUEUE);

    pthread_cond_destroy(&sms_buffer.not_empty);
    pthread_cond_destroy(&sms_buffer.not_full);
    pthread_mutex_destroy(&sms_buffer.mutex);

    puts("SMS stopped successfully");
    return EXIT_SUCCESS;
}
