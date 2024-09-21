#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/cyw43_arch.h>

#define MAIN_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

#define SIDE_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define SIDE_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

SemaphoreHandle_t semaphore = NULL;
StaticSemaphore_t semaphore_buffer;

struct TaskHandle_t coop_thread;
SemaphoreHandle_t semaphore = NULL;
StaticSemaphore_t semaphore_buffer;
int counter;
int on;

void side_thread(void)
{
	while (1) {
        xTaskDelay(100);
        counter += counter + 1;
		printf("hello world from %s! Count %d\n", "thread", counter);
	}
}

void main_thread(void)
{
	while (1) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
        xTaskDelay(100);
		printf("hello world from %s! Count %d\n", "main", counter++);
        on = !on
	}
}

int main(void)
{
    stdio_init_all();
    hard_assert(cyw43_arch_init() == PICO_OK);
    on = false;
    counter = 0;
    xSemaphoreCreateBinaryStatic( &semaphore_buffer );
    xTaskCreate(main_task, "MainThread",
                MAIN_TASK_STACK_SIZE, NULL, MAIN_TASK_PRIORITY, &task);
    xTaskCreate(main_task, "SideThread",
                SIDE_TASK_STACK_SIZE, NULL, SIDE_TASK_PRIORITY, &task);
    vTaskStartScheduler();
	return 0;
}
