#include <stdio.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/cyw43_arch.h>
#include <unity.h>
#include "loop.h"

// The top test runner thread should have highest priority.
// In FreeRTOS bigger is higher priority, with 0 being the lowest.
#define TEST_RUNNER_PRIORITY ( tskIDLE_PRIORITY + 5UL )

void setUp(void) {}

void tearDown(void) {}

/**************** Activity 0-2 ****************/
void test_loop_blocks(void)
{
    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1, 1);
    int counter = 0;
    xSemaphoreTake(semaphore, portMAX_DELAY);

    int result = do_loop(semaphore, &counter, "test", 10);

    TEST_ASSERT_EQUAL_INT(pdFALSE, result);
    TEST_ASSERT_EQUAL_INT(0, counter);
}

void test_loop_runs(void)
{
    int counter = 0;
    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1, 1);
    int result = do_loop(semaphore, &counter, "test", 10);

    TEST_ASSERT_EQUAL_INT(pdTRUE, result);
    TEST_ASSERT_EQUAL_INT(1, counter);
}

/**************** Activity 3 ****************/
// The runner thread has higher priority that subordinates, so we can take control after they deaflock.
#define LEFT_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define LEFT_TASK_PRIORITY ( TEST_RUNNER_PRIORITY - 1UL )
#define RIGHT_TASK_STACK_SIZE configMINIMAL_STACK_SIZE
#define RIGHT_TASK_PRIORITY ( TEST_RUNNER_PRIORITY - 1UL )

void test_deadlock(void)
{
    TaskHandle_t left_thread, right_thread, sup_thread;
    SemaphoreHandle_t left = xSemaphoreCreateCounting(1, 1);
    SemaphoreHandle_t right = xSemaphoreCreateCounting(1, 1);

    printf("- Creating threads\n");

    struct DeadlockArgs left_args = {left, right, 0, 'a'};
    struct DeadlockArgs right_args = {right, left, 7, 'b'};

    BaseType_t left_status =
        xTaskCreate(deadlock, "Left", LEFT_TASK_STACK_SIZE,
                    (void *)&left_args, LEFT_TASK_PRIORITY, &left_thread);
    BaseType_t right_status =
        xTaskCreate(deadlock, "Right", RIGHT_TASK_STACK_SIZE,
                    (void *)&right_args, RIGHT_TASK_PRIORITY, &right_thread);

    printf("- Created threads\n");
    // Once this times out, this thread will preempt the child threads
    // so we can check their state.
    vTaskDelay(1000);
    printf("- Waited 1000 ticks\n");
    // Note that reading and using the value of the semaphore
    // isn't usually a good idea in real situations.
    // Here we have paused everything and are inspecting it frozen in place.
    TEST_ASSERT_EQUAL_INT(uxSemaphoreGetCount(left), 0);
    TEST_ASSERT_EQUAL_INT(uxSemaphoreGetCount(right), 0);
    // Each counter should only be incremented twice
    TEST_ASSERT_EQUAL_INT(2, left_args.counter);
    TEST_ASSERT_EQUAL_INT(9, right_args.counter);
    printf("- Killing threads\n");
    vTaskDelete(left_thread);
    vTaskDelete(right_thread);
    printf("- Killed threads\n");
}

/**************** Activity 4 ****************/
void test_orphaned(void)
{
    int counter = 1;
    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1, 1);

    int result = orphaned_lock(semaphore, 500, &counter);
    TEST_ASSERT_EQUAL_INT(2, counter);
    TEST_ASSERT_EQUAL_INT(pdTRUE, result);
    TEST_ASSERT_EQUAL_INT(1, uxSemaphoreGetCount(semaphore));

    result = orphaned_lock(semaphore, 500, &counter);
    TEST_ASSERT_EQUAL_INT(3, counter);
    TEST_ASSERT_EQUAL_INT(pdFALSE, result);
    TEST_ASSERT_EQUAL_INT(0, uxSemaphoreGetCount(semaphore));

    result = orphaned_lock(semaphore, 500, &counter);
    TEST_ASSERT_EQUAL_INT(3, counter);
    TEST_ASSERT_EQUAL_INT(pdFALSE, result);
    TEST_ASSERT_EQUAL_INT(0, uxSemaphoreGetCount(semaphore));
}

void test_unorphaned(void)
{
    int counter = 1;
    SemaphoreHandle_t semaphore = xSemaphoreCreateCounting(1, 1);

    int result;
    result = unorphaned_lock(semaphore, 500, &counter);
    TEST_ASSERT_EQUAL_INT(2, counter);
    TEST_ASSERT_EQUAL_INT(pdTRUE, result);
    TEST_ASSERT_EQUAL_INT(1, uxSemaphoreGetCount(semaphore));

    result = unorphaned_lock(semaphore, 500, &counter);
    TEST_ASSERT_EQUAL_INT(3, counter);
    TEST_ASSERT_EQUAL_INT(pdTRUE, result);
    TEST_ASSERT_EQUAL_INT(1, uxSemaphoreGetCount(semaphore));
}

/**************** runner ****************/
void runner_thread(__unused void *args)
{
    for (;;) {
        printf("Start tests\n");
        UNITY_BEGIN();
        RUN_TEST(test_loop_blocks);
        RUN_TEST(test_loop_runs);
        RUN_TEST(test_deadlock);
        RUN_TEST(test_orphaned);
        RUN_TEST(test_unorphaned);

        UNITY_END();
        sleep_ms(10000);
    }
}
int main (void)
{
    stdio_init_all();
    hard_assert(cyw43_arch_init() == PICO_OK);
    xTaskCreate(runner_thread, "TestRunner",
                configMINIMAL_STACK_SIZE, NULL, TEST_RUNNER_PRIORITY, NULL);
    vTaskStartScheduler();
	return 0;
}
