#include "loop.h"

#define SLEEPTIME 5

int do_loop(SemaphoreHandle_t semaphore,
            int *counter,
            const char *src,
            TickType_t timeout)
{
    if (xSemaphoreTake(semaphore, timeout) == pdFALSE)
        return pdFALSE;
    {
        (*counter)++;
        printf("hello world from %s! Count %d\n", src, *counter);
    }
    xSemaphoreGive(semaphore);
    return pdTRUE;
}

void deadlock(void *args)
{
    struct DeadlockArgs *dargs = (struct DeadlockArgs *)args;

    printf("\tinside deadlock %c\n", dargs->id);
    dargs->counter++;
    xSemaphoreTake(dargs->a, portMAX_DELAY);
    {
        dargs->counter++;
        printf("\tinside first lock %c\n", dargs->id);
        vTaskDelay(100);
        printf("\tpost-delay %c\n", dargs->id);
        xSemaphoreTake(dargs->b, portMAX_DELAY);
        {
            printf("\tinside second lock %c\n", dargs->id);
            dargs->counter++;
        }
        xSemaphoreGive(dargs->b);
    }
    xSemaphoreGive(dargs->a);
    vTaskSuspend(NULL);
}

int orphaned_lock(SemaphoreHandle_t semaphore, TickType_t timeout, int *counter)
{
    if (xSemaphoreTake(semaphore, timeout) == pdFALSE)
        return pdFALSE;
    {
        (*counter)++;
        if (*counter % 2) {
            return 0;
        }
        printf("Count %d\n", *counter);
    }
    xSemaphoreGive(semaphore);
    return pdTRUE;
}

int unorphaned_lock(SemaphoreHandle_t semaphore, TickType_t timeout, int *counter)
{
    if (xSemaphoreTake(semaphore, timeout) == pdFALSE)
        return pdFALSE;
    {
        (*counter)++;
        if (!(*counter % 2)) {
            printf("Count %d\n", *counter);
        }
    }
    xSemaphoreGive(semaphore);
    return pdTRUE;
}
