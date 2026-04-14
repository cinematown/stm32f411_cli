#include "uart.h"
#include "stm32f4xx_hal_uart.h"
#include <stdarg.h>
#include <stdio.h>

//extern UART_HandleTypeDef huart2;

#define TIMEOUT 1000

#define UART_RX_BUF_LENGTH 256

static uint8_t rx_buf[UART_RX_BUF_LENGTH];
static uint32_t rx_buf_head = 0;
static uint32_t rx_buf_tail = 0;
static uint8_t rx_data;


bool uartInit(void)
{
    bool ret = uartOpen(0,9600);
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);    
    //return uartOpen(0,9600);
    return ret;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
    if(huart->Instance == USART2){
        rx_buf[rx_buf_head] = rx_data;
        rx_buf_head = (rx_buf_head+1) % UART_RX_BUF_LENGTH;

        //다시 인터럽트 활성화 시켜야함 자동x
        HAL_UART_Receive_IT(&huart2, &rx_data, 1);
    }
}

uint32_t uartAvailable(uint8_t ch){
    uint32_t ret = 0; //--debug 초기화

    if(rx_buf_head != rx_buf_tail){
        if(rx_buf_head > rx_buf_tail){
            ret = rx_buf_head - rx_buf_tail;
        }else{
            ret = UART_RX_BUF_LENGTH - (rx_buf_tail - rx_buf_head);
        }
    }

    return ret;
}

uint8_t uartRead(uint8_t ch)
{
    uint8_t ret=0;

    if(rx_buf_head != rx_buf_tail){
        ret = rx_buf[rx_buf_tail];
        rx_buf_tail = (rx_buf_tail+1) % UART_RX_BUF_LENGTH;
    }

    return ret;
}


bool uartOpen(uint8_t ch, uint32_t baudrate)
{
    if(huart2.Init.BaudRate!=baudrate){
        huart2.Init.BaudRate=baudrate;
    }

    if(HAL_UART_DeInit(&huart2) != HAL_OK)
        return false;

    if(HAL_UART_Init(&huart2) != HAL_OK)
        return false;


    return true;
}

bool uartClose(uint8_t ch)
{
    return true;
}


uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t len)
{
   if( HAL_UART_Transmit(&huart2, p_data, len, TIMEOUT) == HAL_OK ){
        return len;
   }

   return 0;
}


uint32_t uartPrintf(uint8_t ch, const char *fmt, ...){

    char buf[128];
    uint32_t len;
    va_list args;

    va_start(args, fmt);
    len = vsnprintf(buf, 128, fmt, args);
    va_end(args);

    return uartWrite(ch, (uint8_t*)buf, len);
}