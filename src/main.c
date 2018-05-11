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

static nbModu_config nbModuConfig = 
{
  .ip = {180, 101, 147, 115},
  .port = 5683,
  .sendMode = 0,
};

int ATIPPORT_cmd(char *cmd, int len)
{
  char buf[128] = "";
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATIPPORT) == 0)
  {
    snprintf(buf, sizeof(buf), "+IPPORT:%d.%d.%d.%d,%d\r\n", 
             nbModuConfig.ip[0], nbModuConfig.ip[1], nbModuConfig.ip[2], 
             nbModuConfig.ip[3], nbModuConfig.port);
    usart_write(USART3, buf, strlen(buf));
  }
  else if(memcmp(buf, ATIPPORTEQ, strlen(ATIPPORTEQ)) == 0)
  {
    int ip[4];
    int port;
    
    if(checkConfigIPORT(buf+strlen(ATIPPORTEQ), ip, &port) == 0)
    {
      for(int i=0; i<sizeof(nbModuConfig.ip)/sizeof(nbModuConfig.ip[0]); i++)
      {
        nbModuConfig.ip[i] = ip[i];
      }
      
      nbModuConfig.port = port;
      
      usart_write(USART3, OKSTR, strlen(OKSTR));
    }
    else
    {
      usart_write(USART3, ERROR, strlen(ERROR));
    }
  }
  else
  {
    usart_write(USART3, ERROR, strlen(ERROR));
  }
  
  return 0;
}

int ATSEMO_cmd(char *cmd, int len)
{
  char buf[128] = "";
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATSEMO) == 0)
  {
    snprintf(buf, sizeof(buf), "+SEMO:%d\r\n", 
             nbModuConfig.sendMode);
    usart_write(USART3, buf, strlen(buf));
  }
  else if(memcmp(buf, ATSEMOEQ, strlen(ATSEMOEQ)) == 0)
  {
    int sendMode = -1;

    if(sscanf(buf+strlen(ATSEMOEQ), "%d", &sendMode) == 1)
    {
      if(sendMode <0 || sendMode>1)
      {
        usart_write(USART3, ERROR, strlen(ERROR));
      }
      else
      {
        nbModuConfig.sendMode = sendMode;
        usart_write(USART3, OKSTR, strlen(OKSTR));
      }
    }
    else
    {
      usart_write(USART3, ERROR, strlen(ERROR));
    }
  }
  else
  {
    usart_write(USART3, ERROR, strlen(ERROR));
  }
  
  return 0;
}

int ATSAVE_cmd(char *cmd, int len)
{
  
  return 0;
}

int ATDELO_cmd(char *cmd, int len)
{
  
  return 0;
}

cmdExcute cmdExe[] = 
{
  {ATIPPORT, ATIPPORT_cmd},
  {ATSEMO, ATSEMO_cmd},
  {ATSAVE, ATSAVE_cmd},
  {ATDELO, ATDELO_cmd},
};





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


void ghConfig()
{
  int ret = 0;
  uint8_t *pStart = NULL;
  uint8_t *pEnd = NULL;
  
  memset(l_uartBuf, 0, sizeof(l_uartBuf));
  l_cnt = 0;
  
  while(1)
  {
    ret = usart_read(USART3, l_uartBuf+l_cnt, sizeof(l_uartBuf)-l_cnt, 10);
    l_cnt += ret;
    
    if(l_cnt == 0)
      continue;
      
    pEnd = memmem(l_uartBuf, l_cnt, ENDFLAG, strlen(ENDFLAG));
    if(pEnd != NULL)
    {
      for(int i=0; i<sizeof(cmdExe)/sizeof(cmdExe[0]); i++)
      {
        pStart = memmem(l_uartBuf, l_cnt, cmdExe[i].cmd, strlen(cmdExe[i].cmd));
        if(pStart == l_uartBuf)
        {
          cmdExe[i].ce_fun((char *)pStart, pEnd - pStart);
          break;
        }
        else if(sizeof(cmdExe)/sizeof(cmdExe[0]) <= i+1) //未定义命令
        {
          usart_write(USART3, UDCMD, strlen(UDCMD));
        }
      }
      
      memset(l_uartBuf, 0, sizeof(l_uartBuf));
      l_cnt = 0;
      //清除串口缓冲
      if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
        USART_ReceiveData(USART3);
    }
    
    if(l_cnt >= sizeof(l_uartBuf))
    {
      memset(l_uartBuf, 0, sizeof(l_uartBuf));
      l_cnt = 0;
      usart_write(USART3, ERROR, strlen(ERROR));
      //清除串口缓冲
      if(USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET)
        USART_ReceiveData(USART3);
    }
    
  }
  
  
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
    ghConfig();
  }

  
  int ret;
  memset(l_uartBuf, 0, sizeof(l_uartBuf));
  l_cnt = 0;
  
  while(mode == lucTrans)
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







