#include <pthread.h>
#include <stdio.h>

static void *worker(void *arg)
{
    const char *name = arg;
    printf("Thread %s started\n", name);
    return NULL;
}

int main(void)
{
    pthread_t threads[4];
    const char *names[] = {
        "Sensor", "Processor", "Alert", "Logger"
    };

    for (int i = 0; i < 4; i++) {
        if (pthread_create(&threads[i], NULL,
                           worker, (void *)names[i]) != 0) {
            fprintf(stderr, "Cannot create thread %d\n", i);
            return 1;
        }
    }

    for (int i = 0; i < 4; i++)
        pthread_join(threads[i], NULL);

    puts("All threads finished");
    return 0;
}
