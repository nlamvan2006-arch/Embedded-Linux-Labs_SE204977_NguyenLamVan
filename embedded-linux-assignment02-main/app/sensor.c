#include "sms_app.h"
#include <sys/ioctl.h>
#include "../driver/sms_sensor.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

static int parse_sample(const char *line, sensor_sample_t *sample)
{
    unsigned long long timestamp;
    double temperature, humidity;

    if (sscanf(line, "%llu,%lf,%lf",
               &timestamp, &temperature, &humidity) != 3)
        return -1;

    sample->timestamp_ms = (uint64_t)timestamp;
    sample->temperature = temperature;
    sample->humidity = humidity;
    return 0;
}

void *sensor_thread(void *arg)
{
    (void)arg;
    int fd = open("/dev/sms_sensor", O_RDONLY);

    if (fd < 0) {
        perror("open sensor");
        return NULL;
    }

    if (ioctl(fd, SMS_SET_INTERVAL, &sms_config.interval_ms) < 0) {
        perror("SMS_SET_INTERVAL");
        close(fd);
        return NULL;
    }

    printf("Sensor interval set: %u ms\\n",
           sms_config.interval_ms);

    while (sms_running) {
        char line[128] = {0};
        sensor_sample_t sample;
        ssize_t n = read(fd, line, sizeof(line) - 1);

        if (n < 0) {
            if (errno == EAGAIN || errno == EINTR) {
                usleep(10000);
                continue;
            }
            perror("read sensor");
            break;
        }

        if (n > 0 && parse_sample(line, &sample) == 0) {
            if (sms_config.verbose) {
                printf("Sensor: %llu, %.2f C, %.2f %%\n",
                       (unsigned long long)sample.timestamp_ms,
                       sample.temperature, sample.humidity);
            }

            pthread_mutex_lock(&sms_buffer.mutex);

            while (sms_buffer.count == SMS_BUFFER_SIZE && sms_running) {
                pthread_cond_wait(&sms_buffer.not_full, &sms_buffer.mutex);
            }

            if (sms_running) {
                sms_buffer.data[sms_buffer.head] = sample;
                sms_buffer.head =
                    (sms_buffer.head + 1) % SMS_BUFFER_SIZE;
                sms_buffer.count++;

                pthread_cond_signal(&sms_buffer.not_empty);
            }

            pthread_mutex_unlock(&sms_buffer.mutex);
        }

        usleep(sms_config.interval_ms * 1000U);
    }

    close(fd);
    return NULL;
}
