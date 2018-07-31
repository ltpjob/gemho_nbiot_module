#include "time_utils.h"
#include "usart_utils.h"


typedef struct tag_uart_handle
{
	rt_device_t hDev;
	uart_rs485 rs485;
	uint8_t rs485_enable;
}uart_handle;


void *usart_init(const char *name, uint32_t USART_BaudRate, 
               uint8_t stopbig, uint8_t parity, uart_rs485 *rs485)
{
	uart_handle *handle = NULL;
	rt_device_t hDev = NULL;
	
	handle = rt_malloc(sizeof(uart_handle));
	
	if(handle == NULL)
		goto __err1;
	
	hDev = rt_device_find(name);
	if (hDev != RT_NULL)
	{
		if (RT_EOK == rt_device_open(hDev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX |
                          RT_DEVICE_FLAG_INT_TX))
		{
			handle->hDev = hDev;
			if(usart_configure(handle, USART_BaudRate, stopbig, parity) != 0)
			{
				rt_device_close(hDev);
				hDev = NULL;
			}
		}
		else
		{
			hDev = NULL;
		}

	}
	
	if(hDev == NULL)
	{
		rt_free(handle);
		handle = NULL;
		goto __err1;
	}
	else
	{
		handle->hDev = hDev;
		if(rs485 == NULL)
		{
			handle->rs485_enable = 0;
		}
		else
		{
			handle->rs485_enable = 1;
			handle->rs485.GPIOx = rs485->GPIOx;
			handle->rs485.GPIO_Pin = rs485->GPIO_Pin;
			GPIO_ResetBits(handle->rs485.GPIOx, handle->rs485.GPIO_Pin);
		}
	}
  
	return handle;
	
__err1:
		
	return NULL;
}

int usart_configure(void *USARTx, uint32_t USART_BaudRate, 
               uint8_t stopbig, uint8_t parity)
{
	if(USARTx == NULL)
		return -1;
	
	uart_handle *handle = USARTx;
	
	rt_device_t hDev = handle->hDev;
	
	uint16_t USART_StopBits;    
	uint16_t USART_Parity;      
	uint16_t USART_WordLength; 
  
  USART_WordLength = DATA_BITS_8;
  
  switch(stopbig)
  {
  case 1:
    USART_StopBits = STOP_BITS_1;
    break;
    
  case 2:
    USART_StopBits = STOP_BITS_2;
    break;
    
  default:
    USART_StopBits = STOP_BITS_1;
  }
  
  switch(parity)
  {
  case 0:
    USART_Parity = PARITY_NONE;
    break;
    
  case 1:
    USART_Parity = PARITY_EVEN;
    USART_WordLength = DATA_BITS_9;
    break;
    
  case 2:
    USART_Parity = PARITY_ODD;
    USART_WordLength = DATA_BITS_9;
    break;
    
  default:
    USART_Parity = PARITY_NONE;
  }
	
	struct serial_configure cfg;
	int ret = 0;
	
	cfg.baud_rate = USART_BaudRate;
	cfg.stop_bits = USART_StopBits;
	cfg.parity = USART_Parity;
	cfg.data_bits = USART_WordLength;
	cfg.bufsz = RT_SERIAL_RB_BUFSZ;
	
	ret = rt_device_control(hDev, RT_DEVICE_CTRL_CONFIG, &cfg);
	
	return ret;
}

int usart_write(void* USARTx, const void *d, size_t len)
{
	if(USARTx == NULL)
		return -1;
	
	uart_handle *handle = USARTx;
	rt_device_t hDev = handle->hDev;
	
	int ret = 0;
	
	if(handle->rs485_enable == 1)
		GPIO_SetBits(handle->rs485.GPIOx, handle->rs485.GPIO_Pin);		
	
	ret = rt_device_write(hDev, 0, d, len);
	
	
	if(handle->rs485_enable == 1)
	{
		rt_thread_delay(rt_tick_from_millisecond(2));
		GPIO_ResetBits(handle->rs485.GPIOx, handle->rs485.GPIO_Pin);
	}
  
  return ret;
}

int usart_read(void* USARTx, void *d, size_t len, int timeout)
{
	if(USARTx == NULL)
		return -1;
	
	uart_handle *handle = USARTx;
	rt_device_t hDev = handle->hDev;
  size_t cnt = 0;
  uint8_t *data = d;
	int ret = 0;
  
  uint64_t uLastRcvTime = get_timestamp();
  
  while(1)
  {
		ret = rt_device_read(hDev, 0, data+cnt, len-cnt);
		if(ret > 0)
		{
			uLastRcvTime = get_timestamp();
			cnt+=ret;
		}
		else if(timeout != 0)
			rt_thread_delay(1);
    
    if((timeout != -1 && get_timestamp()-uLastRcvTime >= timeout) || cnt >= len)
      break;
  }
  
  return cnt;
}

rt_err_t
usart_set_rx_indicate(void* USARTx,
                          rt_err_t (*rx_ind)(rt_device_t dev, rt_size_t size))
{
	if(USARTx == NULL)
	return -1;
	
	uart_handle *handle = USARTx;
	rt_device_t hDev = handle->hDev;
	
	return rt_device_set_rx_indicate(hDev, rx_ind);
	
}

