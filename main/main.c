/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

void task_x(void *p){
    adc_init();
    adc_gpio_init(1);

    while(1) {
        adc_select_input(1);
        int adc_val = adc_read();

        struct adc depois = {
            .axis = 1,
            .val = adc_val
        };
        xQueueSend(xQueueAdc, &depois, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void task_y(void *p){
    adc_init();
    adc_gpio_init(2);

    while(1) {
        adc_select_input(2);
        int adc_val = adc_read();

        struct adc depois = {
            .axis = 2,
            .val = adc_val
        };
        xQueueSend(xQueueAdc, &depois, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis); 
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb); 
    uart_putc_raw(uart0, -1); 
}

void uart_task(void *p) {
    adc_t data;

    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);

        //deadzone
        data.val = (data.val-2047)/8;
        int zone_limit = 80;
        if (data.val <=zone_limit && data.val >= -1*(zone_limit)) {
            data.val = 0;
        }

        write_package(data);
    }
}

int main() {
    stdio_init_all();

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(task_x, "ADC_Task x", 256, NULL, 1, NULL);

    xTaskCreate(task_y, "ADX_Task y", 256, NULL, 1, NULL);

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
