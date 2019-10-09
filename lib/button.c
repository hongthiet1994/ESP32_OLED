#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"



// static xQueueHandle gpio_evt_queue = NULL;

// static void IRAM_ATTR gpio_isr_handler(void* arg)
// {
//     uint32_t gpio_num = (uint32_t) arg;
//     xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
// }

// void app_main()
// {
//     gpio_config_t io_conf;
//     //disable interrupt
//     io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
//     //set as output mode
//     io_conf.mode = GPIO_MODE_OUTPUT;
//     //bit mask of the pins that you want to set,e.g.GPIO18/19
//     io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
//     //disable pull-down mode
//     io_conf.pull_down_en = 0;
//     //disable pull-up mode
//     io_conf.pull_up_en = 0;
//     //configure GPIO with the given settings
//     gpio_config(&io_conf);

//     //interrupt of rising edge
//     io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
//     //bit mask of the pins, use GPIO4/5 here
//     io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
//     //set as input mode    
//     io_conf.mode = GPIO_MODE_INPUT;
//     //enable pull-up mode
//     io_conf.pull_up_en = 1;
//     gpio_config(&io_conf);

//     //change gpio intrrupt type for one pin
//     gpio_set_intr_type(GPIO_INPUT_IO_0, GPIO_INTR_ANYEDGE);

//     //create a queue to handle gpio event from isr
//     gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
//     //start gpio task
//     xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

//     //install gpio isr service
//     gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
//     //hook isr handler for specific gpio pin
//     gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);
//     //hook isr handler for specific gpio pin
//     gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void*) GPIO_INPUT_IO_1);

//     //remove isr handler for gpio number.
//     gpio_isr_handler_remove(GPIO_INPUT_IO_0);
//     //hook isr handler for specific gpio pin again
//     gpio_isr_handler_add(GPIO_INPUT_IO_0, gpio_isr_handler, (void*) GPIO_INPUT_IO_0);

//     int cnt = 0;
//     while(1) {
//         printf("cnt: %d\n", cnt++);
//         vTaskDelay(1000 / portTICK_RATE_MS);
//         gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
//         gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
//     }
// }

