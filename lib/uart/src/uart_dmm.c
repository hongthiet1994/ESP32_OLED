
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "uart_dmm.h"
#include "lwip/sockets.h"
#include "debug.h"
#include "device_infor_dmm.h"
#include "json_dmm.h"



extern uint32_t ui32_Send_WiFi_status_to_STM;

extern uint32_t ui32_receive_cmd_stop;

extern DEVICE_INFOR   Device_Infor;

extern CALIB calib[DMM_CNTSCALES];


extern TaskHandle_t xTask_Uart_event , xTask_Uart_process_data ;
static QueueHandle_t uart0_queue;
extern int sock;
extern struct sockaddr_in6 sourceAddr;

uint8_t buffer_RX_uart[LEN_BUFFER_RX_UART] = {0};
uint8_t buffer_RX_uart_backup[LEN_BUFFER_RX_UART] = {0};
uint32_t ui32_len_total = 0;
uint32_t ui32_len_final = 0;
uint32_t ui32_time_out_receive_uart = 0;



void config_uart_0()
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = 
    {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_set_pin(UART_NUM_0, ECHO_TEST_TXD_0, ECHO_TEST_RXD_0, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);
}

void config_uart_2()
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = 
    {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };    

    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, ECHO_TEST_TXD_2, ECHO_TEST_RXD_2, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);   
}

void free_buffer_RX_uart()
{
    memset(buffer_RX_uart,0,LEN_BUFFER_RX_UART);
}
void free_buffer_RX_uart_backup()
{
    memset(buffer_RX_uart_backup,0,LEN_BUFFER_RX_UART);
}
void check_time_out_receive_uart()
{
    ui32_time_out_receive_uart++;  
    if(ui32_time_out_receive_uart > TIME_OUT_RECEIVE_UART)
    {
       free_buffer_RX_uart();
       ui32_time_out_receive_uart = 0;
       ui32_len_total = 0;
       ESP_LOGI(UART_DEBUG, "TIME OUT");      
    }
}
void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    uint8_t pat[PATTERN_CHR_NUM + 1];
    uint32_t ui32_len_temp = 0;
    for(;;) 
	{
        //Waiting for UART event.
        if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)portMAX_DELAY)) 
        //if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)100)) 
        {
            bzero(dtmp, RD_BUF_SIZE);            
            switch(event.type) 
            {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:  
                    ui32_time_out_receive_uart = 0; 
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    ui32_len_temp = ui32_len_total + event.size; 
                    //ESP_LOGI(UART_DEBUG,"LEN: %d ", event.size); 
                    if(ui32_len_temp>= LEN_BUFFER_RX_UART ) 
                    {
                        free_buffer_RX_uart();
                        ui32_len_total = 0;
                    } 
                    else
                    {
                        memcpy(&buffer_RX_uart[ui32_len_total],dtmp,event.size); 
                        ui32_len_total = ui32_len_temp;                         
                    }                                                                                        
                    #ifdef  DEBUG_UART
                        //ESP_LOGI(UART_DEBUG, "[UART DATA SIZE]: %d", event.size);                
                        //ESP_LOGI(UART_DEBUG, "[DATA RECEIVER]: %s",dtmp);
                        ESP_LOGI(UART_DEBUG, "[UART DATA SIZE TOTAL]: %d", ui32_len_total);  
                        //ESP_LOGI(UART_DEBUG, "[DATA TOTAL]: %s",buffer_RX_uart);
                    #endif                                       
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(UART_DEBUG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_STM32);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(UART_DEBUG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(UART_STM32);
                    xQueueReset(uart0_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGI(UART_DEBUG, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGI(UART_DEBUG, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(UART_DEBUG, "uart frame error");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    uart_get_buffered_data_len(UART_STM32, &buffered_size);
                    int pos = uart_pattern_pop_pos(UART_STM32);
                    //ESP_LOGI(UART_DEBUG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                    if (pos == -1) 
                    {
                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                        // record the position. We should set a larger queue size.
                        // As an example, we directly flush the rx buffer here.
                        uart_flush_input(UART_STM32);
                        xQueueReset(uart0_queue);
                    } 
                    else 
                    {
                        //if(ui32_receive_cmd_stop == false)
                        {
                            ui32_len_temp = ui32_len_total + pos;
                            if(ui32_len_temp>= LEN_BUFFER_RX_UART ) 
                            {

                            } 
                            else
                            {                            
                                uart_read_bytes(UART_STM32, dtmp, pos, 100 / portTICK_PERIOD_MS);
                                uart_read_bytes(UART_STM32, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                                memcpy(&buffer_RX_uart[ui32_len_total],dtmp,pos);
                                ui32_len_total = ui32_len_temp;
                                ui32_len_final = ui32_len_total;                               
                                memcpy(buffer_RX_uart_backup,buffer_RX_uart,ui32_len_total);  
                                xTaskNotifyGive(xTask_Uart_process_data);                          
                                //ESP_LOGI(UART_DEBUG,"LEN: %d ", ui32_len_total);                                 
                                                                                                               
                            }
                            //free_buffer_RX_uart();
                            uart_flush_input(UART_STM32);
                            xQueueReset(uart0_queue);
                            ui32_len_total = 0; 
                        }
                                                                      
                    }                   
                    break;
                //Others
                default:
                    ESP_LOGI(UART_DEBUG, "uart event type: %d", event.type);
                    break;
            }
        }
        //check_time_out_receive_uart();
    }
    free(dtmp);
    dtmp = NULL;    
    vTaskDelete(NULL);
}
void process_data_uart_task(void *pvParameters)
{    
    uint8_t command = 0;   
    for( ;; )
    {       
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);     
        if(buffer_RX_uart_backup[1] == '@')
        {           
            command = buffer_RX_uart_backup[2] - '0';          
            proccess_Command_STM(command);
        }   
        else
        {                    
            sendto(sock, buffer_RX_uart_backup, ui32_len_final, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));                                    
        }             
        free_buffer_RX_uart_backup();    
        vTaskDelay(5/ portTICK_PERIOD_MS);       
    }    
    vTaskDelete(NULL);
}

void config_uart_0_queue()
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    //esp_log_level_set(UART_DEBUG, ESP_LOG_INFO);
    uart_config_t uart_config = 
	{
        .baud_rate = BAUD_RATE_UART_STM,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl =  UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_STM32, &uart_config);
    //Set UART log level
    esp_log_level_set(UART_DEBUG, ESP_LOG_INFO);

    uart_set_hw_flow_ctrl(UART_STM32,UART_HW_FLOWCTRL_CTS_RTS, 255);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(UART_STM32, ECHO_TEST_TXD_0, ECHO_TEST_RXD_0, ECHO_TEST_RTS, ECHO_TEST_CTS);
    //Install UART driver, and get the queue.
    uart_driver_install(UART_STM32, BUF_SIZE * 2, BUF_SIZE * 2, QUEUE_SIZE, &uart0_queue, 0);
    //Set uart pattern detect function.
    uart_enable_pattern_det_intr(UART_STM32,'*', PATTERN_CHR_NUM, 1000, 10, 10);
    //Reset the pattern queue length to record at most QUEUE_SIZE pattern positions.
    uart_pattern_queue_reset(UART_STM32, QUEUE_SIZE);
}

void proccess_Command_STM(uint8_t command)
{ 
    ESP_LOGI(UART_DEBUG, "proccess_Command_STM");
    switch (command)
    {
        case COMMAND_REQUEST_AP_MODE_ESP32:
            //readBufferEEPROM(Device_Infor.BufferDeviceInfor,ADDRESS_EEPROM_DEVICE_INFOR,LEN_BUFFER_DEVICE_INFOR,true);			
            //AccessPointMode(Device_Infor.data.BufferDeviceID,PASS_AP_DEFAULT,CHANNEL_ACCESS_POINT);
            break;
        case COMMAND_SYNC_DEVICE_INFOR_REP: 
            memcpy(Device_Infor.bufferDeviceinfor,&buffer_RX_uart_backup[3],LEN_BUFFER_DEVICE_INFOR);
            memcpy(calib,&buffer_RX_uart_backup[LEN_BUFFER_DEVICE_INFOR+3],DMM_CNTSCALES*16);
            free_buffer_RX_uart_backup();            
            creat_json_device_infor(REP);            
            break;
        case COMMAND_SYNC_DEVICE_INFOR_SEND: 
            memcpy(Device_Infor.bufferDeviceinfor,&buffer_RX_uart_backup[3],LEN_BUFFER_DEVICE_INFOR);
            memcpy(calib,&buffer_RX_uart_backup[LEN_BUFFER_DEVICE_INFOR+3],DMM_CNTSCALES*16);
            free_buffer_RX_uart_backup();
            creat_json_device_infor(SEND);
            break;
        case COMMAND_CONFIRM_WIFI_STATUS:
            ui32_Send_WiFi_status_to_STM = true;
            break;
        default:
            break;
    }   
    //free_RX_Buffer_STM32(); 

}