#ifndef __USART_UTILS_H
#define __USART_UTILS_H

#ifdef __cplusplus
 extern "C" {
#endif 
   
#include <stdio.h>
#include "stm32f10x.h"

typedef struct tag_uart_rs485
{
	GPIO_TypeDef* GPIOx;
	uint16_t GPIO_Pin;
}uart_rs485;

void *usart_init(const char *name, uint32_t USART_BaudRate, 
						 uint8_t stopbig, uint8_t parity, uart_rs485 *rs485);

int usart_configure(void *USARTx, uint32_t USART_BaudRate, 
               uint8_t stopbig, uint8_t parity);
 
int usart_write(void* USARTx, const void *d, size_t len);

int usart_read(void* USARTx, void *d, size_t len, int timeout);
   
   
   
   
   
   
#ifdef __cplusplus
 }
#endif

#endif /* __USART_UTILS_H */





