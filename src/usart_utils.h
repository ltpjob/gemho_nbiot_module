#ifndef __USART_UTILS_H
#define __USART_UTILS_H

#ifdef __cplusplus
 extern "C" {
#endif 
   
#include <stdio.h>
#include "stm32f10x.h"
#include <rtdevice.h>

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
   
rt_err_t
usart_set_rx_indicate(void* USARTx,
                          rt_err_t (*rx_ind)(rt_device_t dev, rt_size_t size));
   
   
   
   
#ifdef __cplusplus
 }
#endif

#endif /* __USART_UTILS_H */





