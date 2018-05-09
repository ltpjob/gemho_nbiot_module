#ifndef __USART_UTILS_H
#define __USART_UTILS_H

#ifdef __cplusplus
 extern "C" {
#endif 
   
#include <stdio.h>
#include "stm32f10x.h"

   
 
size_t usart_write(USART_TypeDef* USARTx, const void *d, size_t len);

size_t usart_read(USART_TypeDef* USARTx, void *d, size_t len, int timeout);
   
   
   
   
   
   
#ifdef __cplusplus
 }
#endif

#endif /* __USART_UTILS_H */