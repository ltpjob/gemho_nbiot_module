#include "usart_utils.h"
#include "time_utils.h"


size_t usart_write(USART_TypeDef* USARTx, const void *d, size_t len)
{
  const uint8_t *data = d;
  
  for(int i=0; i<len; i++)
  {
    USART_SendData(USARTx, data[i]);
    while(USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET){}
  }
  
  return len;
}

size_t usart_read(USART_TypeDef* USARTx, void *d, size_t len, int timeout)
{
  size_t cnt = 0;
  uint8_t *data = d;
  
  uint64_t uLastRcvTime = get_timestamp();
  
  while(1)
  {
    if(USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) != RESET)
    {
      uLastRcvTime = get_timestamp();
      data[cnt++] = USART_ReceiveData(USARTx);
    }
    
    if((timeout != -1 && get_timestamp()-uLastRcvTime > timeout) || cnt >= len)
      break;
  }
  
  return cnt;
}