#include "sms_app.h"
#include <stdio.h>
#include <math.h>

void *processor_thread(void *arg)
{
    (void)arg;

    sensor_sample_t history[SMS_AVG_WINDOW] = {0};
    unsigned int history_count = 0;
    unsigned int history_pos = 0;
    double previous_temperature = 0.0;

    while (sms_running) {
        sensor_sample_t sample;

        pthread_mutex_lock(&sms_buffer.mutex);

        while (sms_buffer.count == 0 && sms_running)
            pthread_cond_wait(&sms_buffer.not_empty,
                              &sms_buffer.mutex);

        if (!sms_running) {
            pthread_mutex_unlock(&sms_buffer.mutex);
            break;
        }

        sample = sms_buffer.data[sms_buffer.tail];
        sms_buffer.tail =
            (sms_buffer.tail + 1) % SMS_BUFFER_SIZE;
        sms_buffer.count--;

        pthread_cond_signal(&sms_buffer.not_full);
        pthread_mutex_unlock(&sms_buffer.mutex);

        history[history_pos] = sample;
        history_pos = (history_pos + 1) % SMS_AVG_WINDOW;

        if (history_count < SMS_AVG_WINDOW)
            history_count++;

        double temp_sum = 0.0;
        double humid_sum = 0.0;

        for (unsigned int i = 0; i < history_count; i++) {
            temp_sum += history[i].temperature;
            humid_sum += history[i].humidity;
        }

        double avg_temp = temp_sum / history_count;
        double avg_humid = humid_sum / history_count;

        if (sms_config.verbose)
            printf("Processor: avg_temp=%.2f avg_humid=%.2f\n",
                   avg_temp, avg_humid);

        int flags = 0;

        if (sample.temperature > sms_config.temp_threshold)
            flags |= SMS_ALERT_TEMP;

        if (sample.humidity > sms_config.humid_threshold)
            flags |= SMS_ALERT_HUMID;

        if (history_count > 1 &&
            (sample.temperature - previous_temperature) > 5.0)
            flags |= SMS_ALERT_SPIKE;

        previous_temperature = sample.temperature;

        if (flags && sms_config.verbose)
            printf("ALERT: flags=%d temp=%.2f humid=%.2f\n",
                   flags, sample.temperature, sample.humidity);

        sms_message_t msg = {
            .timestamp_ms = sample.timestamp_ms,
            .temperature = sample.temperature,
            .humidity = sample.humidity,
            .avg_temperature = avg_temp,
            .avg_humidity = avg_humid,
            .alert_flags = flags
        };

        if (mq_send(sms_log_mq, (const char *)&msg,
                    sizeof(msg), 0) == -1)
            perror("mq_send log");

        if (flags != 0 &&
            mq_send(sms_alert_mq, (const char *)&msg,
                    sizeof(msg), 0) == -1)
            perror("mq_send alert");
    }

    return NULL;
}
