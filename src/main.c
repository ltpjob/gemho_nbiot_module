#include <stdio.h>
#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#include <string.h>
#include "gemho_cmd.h"
#include "utils.h"
#include "time_utils.h"
#include "usart_utils.h"


static uint8_t l_uartBuf[1024] = "";
static char l_sendBuf[2048] = "";
static uint32_t l_cnt = 0;

typedef enum ModeToRun_tag{
  atDebug = 0,
  gemhoConfig,
  lucTrans,
}ModeToRun; 


//时钟设置
void RCC_Configuration(void)
{
  /* Enable GPIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_USART1, ENABLE);
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

}

//io设置
void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  //usart1
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  
    //usart3
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
}

//usart1直连usart3
void usart3_direct_usart1()
{
  while(1)
  {
    uint16_t data;
    
    if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
    {
      data = USART_ReceiveData(USART1);
      USART_SendData(USART3,data);
      while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET){}
    }
    
    if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
    {
      data = USART_ReceiveData(USART3);
      USART_SendData(USART1,data);
      while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET){}
    }
  }
}



int coap_msgSend(uint8_t *data, uint32_t len)
{
  uint32_t count = 0;
  
  if(len >COAP_MAXLEN)
    len = COAP_MAXLEN;
  
  if(len == 0 || sizeof(l_sendBuf) < len*2+30)
    return 0;
  
  
  memset(l_sendBuf, 0, sizeof(l_sendBuf));
  count = snprintf(l_sendBuf, sizeof(l_sendBuf), "AT+NMGS=%d,00%04X", len+3, len);
  for(int i=0; i<len; i++)
  {
    count += snprintf(l_sendBuf+count, sizeof(l_sendBuf)-count, "%02X", data[i]);
  }
  count += snprintf(l_sendBuf+count, sizeof(l_sendBuf)-count, "\r\n");
  
//  printf(l_sendBuf);
  
  usart_write(USART1, l_sendBuf, strlen(l_sendBuf));
  
  return 0;
}

//开机模式选择
ModeToRun start_mode()
{
  uint64_t uLastRcvTime = 0;
  ModeToRun mode = lucTrans;
  
  memset(l_uartBuf, 0, sizeof(l_uartBuf));
  l_cnt = 0;
  
  while(1)
  { 
    if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
    {
      uLastRcvTime = get_timestamp();
      l_uartBuf[l_cnt++] = USART_ReceiveData(USART3);
    }
    
    if((l_cnt > 0 && get_timestamp()-uLastRcvTime > 100) || l_cnt >= sizeof(l_uartBuf))
    {
      if(memmem(l_uartBuf, l_cnt, ATDEBUG, strlen(ATDEBUG)) != NULL)
      {
        mode = atDebug;
        usart_write(USART3, ATDEBUG, strlen(ATDEBUG));
        
        break;
      }
      else if(memmem(l_uartBuf, l_cnt, GEMHOCFG, strlen(GEMHOCFG)) != NULL)
      {
        mode = gemhoConfig;
        usart_write(USART3, GEMHOCFG, strlen(GEMHOCFG));
        
        break;
      }
      else
      {
        mode = lucTrans;
        coap_msgSend(l_uartBuf, l_cnt);
        break;
      }
    }
    
  }
  
  return mode;
}

int main(void)
{
  SystemInit();
  RCC_Configuration();
  GPIO_Configuration();
  tick_ms_init();
  
  USART_InitTypeDef USART_InitStructure;
  
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  
  
  USART_Cmd(USART1, DISABLE);
  USART_Init(USART1, &USART_InitStructure);
  USART_Cmd(USART1, ENABLE);
  
  USART_Cmd(USART3, DISABLE);
  USART_Init(USART3, &USART_InitStructure);
  USART_Cmd(USART3, ENABLE);
  
  ModeToRun mode = start_mode();
  
  if(mode == atDebug)
  {
    usart3_direct_usart1();
  }
  else if(mode == gemhoConfig)
  {
    usart3_direct_usart1();
  }
  else if(mode == lucTrans)
  {
    int ret = 0;
    
    memset(l_uartBuf, 0, sizeof(l_uartBuf));
    l_cnt = 0;
    
    while(1)
    {
      ret = usart_read(USART3, l_uartBuf, sizeof(l_uartBuf), 100);
      if(ret > 0)
      {
        coap_msgSend(l_uartBuf, ret);
        memset(l_uartBuf, 0, sizeof(l_uartBuf));
        l_cnt = 0;
        
        //清除串口缓冲
        if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
          USART_ReceiveData(USART3);
      }
    }
  }
 
}







