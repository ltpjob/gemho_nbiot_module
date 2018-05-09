#include <stdio.h>
#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#include <string.h>
#include "gemho_cmd.h"

static uint8_t l_uartBuf[1024] = "";
static char l_sendBuf[2048] = "";
static uint32_t l_cnt = 0;

typedef enum ModeToRun_tag{
  atDebug = 0,
  gemhoConfig,
  lucTrans,
}ModeToRun; 

static volatile uint64_t l_timestamp = 0;

static void *memmem(const void *l, size_t l_len, const void *s, size_t s_len)  
{  
    register char *cur, *last;  
    const char *cl = (const char *)l;  
    const char *cs = (const char *)s;  
   
    /* we need something to compare */  
    if (l_len == 0 || s_len == 0)  
        return NULL;  
   
    /* "s" must be smaller or equal to "l" */  
    if (l_len < s_len)  
        return NULL;  
   
    /* special case where s_len == 1 */  
    if (s_len == 1)  
        return memchr(l, (int)*cs, l_len);  
   
    /* the last position where its possible to find "s" in "l" */  
    last = (char *)cl + l_len - s_len;  
   
    for (cur = (char *)cl; cur <= last; cur++)  
        if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)  
            return cur;  
   
    return NULL;  
}  

//获取时间戳，单位ms
uint64_t get_timestamp()
{
  return l_timestamp;
}

//systick初始化
void tick_ms_init()
{
  if (SysTick_Config(SystemCoreClock / 1000))
  {
    /* Capture error */ 
    while (1);
  }
}

//systick中断
void SysTick_Handler(void)
{
  l_timestamp++;
}

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

//int atSend(uint8_t *data, uint32_t len, int timeout)
//{
//  uint64_t uLastRcvTime = 0;
//  uint64_t uSendFinTime = 0;
//  int status = 0;
//  
//  memset(l_uartBuf, 0, sizeof(l_uartBuf));
//  l_cnt = 0;
//  
//  for(int i=0; i<len; i++)
//  {
//    USART_SendData(USART1, data[i]);
//    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET){}
//  }
//  
//  uSendFinTime = get_timestamp();
//  
//  while(1)
//  {
//    if(get_timestamp() - uSendFinTime>=timeout)
//    {
//      status = -2;
//      break;
//    }
//    
//    if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
//    {
//      uLastRcvTime = get_timestamp();
//      l_uartBuf[l_cnt++] = USART_ReceiveData(USART3);
//    }
//    
//    if(l_cnt > 0 && get_timestamp()-uLastRcvTime > 100)
//    {
//      if(memmem(l_uartBuf, l_cnt, ATDEBUG, strlen(ATDEBUG)) != NULL)
//      {
//        mode = atDebug;
//        memset(l_uartBuf, 0, sizeof(l_uartBuf));
//        l_cnt = 0;
//        break;
//      }
//      else if(memmem(l_uartBuf, l_cnt, GEMHOCFG, strlen(GEMHOCFG)) != NULL)
//      {
//        mode = gemhoConfig;
//        memset(l_uartBuf, 0, sizeof(l_uartBuf));
//        l_cnt = 0;
//        break;
//      }
//      else
//      {
//        mode = lucTrans;
//        coap_msgSend(l_uartBuf, l_cnt);
//        memset(l_uartBuf, 0, sizeof(l_uartBuf));
//        l_cnt = 0;
//        break;
//      }
//    }
//  }
//}

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
  
  for(int i=0; i<strlen(l_sendBuf); i++)
  {
    USART_SendData(USART1, l_sendBuf[i]);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET){}
  }
  
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
        memset(l_uartBuf, 0, sizeof(l_uartBuf));
        l_cnt = 0;
        
        for(int i=0; i<strlen(ATDEBUG); i++)
        {
          USART_SendData(USART3, ATDEBUG[i]);
          while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET){}
        }
        
        break;
      }
      else if(memmem(l_uartBuf, l_cnt, GEMHOCFG, strlen(GEMHOCFG)) != NULL)
      {
        mode = gemhoConfig;
        memset(l_uartBuf, 0, sizeof(l_uartBuf));
        l_cnt = 0;
        
        for(int i=0; i<strlen(GEMHOCFG); i++)
        {
          USART_SendData(USART3, GEMHOCFG[i]);
          while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET){}
        }
        
        break;
      }
      else
      {
        mode = lucTrans;
        coap_msgSend(l_uartBuf, l_cnt);
        memset(l_uartBuf, 0, sizeof(l_uartBuf));
        l_cnt = 0;
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
    uint64_t uLastRcvTime = 0;
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
        coap_msgSend(l_uartBuf, l_cnt);
        memset(l_uartBuf, 0, sizeof(l_uartBuf));
        l_cnt = 0;
        
        //清除串口缓冲
        if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
          USART_ReceiveData(USART3);
      }
    }
  }
 
}