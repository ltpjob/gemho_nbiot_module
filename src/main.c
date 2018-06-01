#include <stdio.h>
#include "stm32f10x.h"
#include <string.h>
#include "gemho_nbiot_module.h"
#include "utils.h"
#include "time_utils.h"
#include "usart_utils.h"
#include "config_utils.h"
#include <rtthread.h>
#include "msg_fifo.h"


//static uint8_t l_uartBuf[1024] = "";
static char l_sendBuf[2048] = "";
//static uint32_t l_cnt = 0;


static void *USERCOM = NULL;
static void *BC95COM = NULL;
static void *l_hMsgFifo = NULL;
static rt_sem_t sem_urx = RT_NULL;

static const nbModu_config l_default_nbMCFG = 
{
  .ip = {180, 101, 147, 115},
  .port = 5683,
  .sendMode = 0,
  .baudrate = 115200,
  .stopbit = 1,
  .parity = 0,
};


static nbModu_config l_nbModuConfig;


static char l_IMEI[15] = "";
static int l_nband = 0;

//baudrate设置
static int ATRS232_cmd(char *cmd, int len)
{
  char buf[128] = "";
  uint32_t baudrate;
  uint8_t stopbit;
  uint8_t parity;
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATRS232) == 0)
  {
    snprintf(buf, sizeof(buf), "+RS232:%d,%d,%d\r\n", 
             l_nbModuConfig.baudrate, l_nbModuConfig.stopbit, l_nbModuConfig.parity);
    usart_write(USERCOM, buf, strlen(buf));
  }
  else if(memcmp(buf, ATRS232EQ, strlen(ATRS232EQ)) == 0)
  {
    if(checkConfigRS232(buf+strlen(ATRS232EQ), &baudrate, &stopbit, &parity) == 0)
    {
      l_nbModuConfig.baudrate = baudrate;
      l_nbModuConfig.stopbit = stopbit;
      l_nbModuConfig.parity = parity;
      
      usart_write(USERCOM, OKSTR, strlen(OKSTR));
    }
    else
    {
      usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    }
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return 0;
}

//ip port设置
static int ATIPPORT_cmd(char *cmd, int len)
{
  char buf[128] = "";
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATIPPORT) == 0)
  {
    snprintf(buf, sizeof(buf), "+IPPORT:%d.%d.%d.%d,%d\r\n", 
             l_nbModuConfig.ip[0], l_nbModuConfig.ip[1], l_nbModuConfig.ip[2], 
             l_nbModuConfig.ip[3], l_nbModuConfig.port);
    usart_write(USERCOM, buf, strlen(buf));
  }
  else if(memcmp(buf, ATIPPORTEQ, strlen(ATIPPORTEQ)) == 0)
  {
    int ip[4];
    int port;
    
    if(checkConfigIPORT(buf+strlen(ATIPPORTEQ), ip, &port) == 0)
    {
      for(int i=0; i<sizeof(l_nbModuConfig.ip)/sizeof(l_nbModuConfig.ip[0]); i++)
      {
        l_nbModuConfig.ip[i] = ip[i];
      }
      
      l_nbModuConfig.port = port;
      
      usart_write(USERCOM, OKSTR, strlen(OKSTR));
    }
    else
    {
      usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    }
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return 0;
}

//获取imei，nband
static int ATIMEIBD_cmd(char *cmd, int len)
{
  char buf[64] = "";
  char IMEI[32] = "";
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATIMEIBD) == 0)
  {
    memcpy(IMEI, l_IMEI, sizeof(l_IMEI));
    snprintf(buf, sizeof(buf), "+IMEIBD:%s,%d\r\n", IMEI, l_nband);
    usart_write(USERCOM, buf, strlen(buf));
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return 0;
}

//设置模式(coap or udp)
static int ATSEMO_cmd(char *cmd, int len)
{
  char buf[128] = "";
  
  if(len > sizeof(buf)-1)
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    return 0;
  }
  
  memcpy(buf, cmd, len);
  
  if(strcmp(buf, ATSEMO) == 0)
  {
    snprintf(buf, sizeof(buf), "+SEMO:%d\r\n", 
             l_nbModuConfig.sendMode);
    usart_write(USERCOM, buf, strlen(buf));
  }
  else if(memcmp(buf, ATSEMOEQ, strlen(ATSEMOEQ)) == 0)
  {
    int sendMode = -1;

    if(sscanf(buf+strlen(ATSEMOEQ), "%d", &sendMode) == 1)
    {
      if(sendMode <0 || sendMode>1)
      {
        usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
      }
      else
      {
        l_nbModuConfig.sendMode = sendMode;
        usart_write(USERCOM, OKSTR, strlen(OKSTR));
      }
    }
    else
    {
      usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    }
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return 0;
}


//保存配置
static int ATSAVE_cmd(char *cmd, int len)
{
  int ret = 0;
  
  for(int i=0; i<3; i++)
  {
    ret = save_config(&l_nbModuConfig);
    if(ret == 0)
      break;
  }
  
  if(ret == 0)
  {
    usart_write(USERCOM, OKSTR, strlen(OKSTR));
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return ret;
}


//恢复默认配置
static int ATDELO_cmd(char *cmd, int len)
{
  int ret = 0;
  
  memcpy(&l_nbModuConfig, &l_default_nbMCFG, sizeof(l_nbModuConfig));
    
  for(int i=0; i<3; i++)
  {
    ret = save_config(&l_nbModuConfig);
    if(ret == 0)
      break;
  }
  
  if(ret == 0)
  {
    usart_write(USERCOM, OKSTR, strlen(OKSTR));
  }
  else
  {
    usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
  }
  
  return ret;
}


static cmdExcute cmdExe[] = 
{
  {ATRS232, ATRS232_cmd},
  {ATIPPORT, ATIPPORT_cmd},
  {ATIMEIBD, ATIMEIBD_cmd},
  {ATSEMO, ATSEMO_cmd},
  {ATSAVE, ATSAVE_cmd},
  {ATDELO, ATDELO_cmd},
};



//时钟设置
static void RCC_Configuration(void)
{
  /* Enable GPIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_USART1 | RCC_APB2Periph_AFIO, ENABLE);
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3 | RCC_APB1Periph_I2C1, ENABLE);

}

//io设置
static void GPIO_Configuration(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  //BC95COM
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  
    //USERCOM
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
    //i2c1
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  //BC95/RST
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  //led_R led_Y
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  
  //key
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
}

static void RST_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource13);

  /* Configure EXTI0 line */
  EXTI_InitStructure.EXTI_Line = EXTI_Line13;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;  
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set EXTI0 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}


//BC95COM直连USERCOM
static void USERCOM_direct_BC95COM()
{
  while(1)
  {
    char buf[64]="";
		int ret = 0;
    
    IWDG_Feed();
    
		ret = usart_read(USERCOM, buf, sizeof(buf), 0);
		if(ret > 0)
		{
			usart_write(BC95COM, buf, ret);
		}
		
		ret = usart_read(BC95COM, buf, sizeof(buf), 0);
		if(ret > 0)
		{
			usart_write(USERCOM, buf, ret);
		}
  }
}


//coap发送
static int coap_msgSend(uint8_t *data, uint32_t len)
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
  
  usart_write(BC95COM, l_sendBuf, strlen(l_sendBuf));
  
  return 0;
}


//udp发送
static int udp_msgSend(uint8_t *data, uint32_t len)
{
  uint32_t count = 0;
  
  if(len >COAP_MAXLEN)
    len = COAP_MAXLEN;
  
  if(len == 0 || sizeof(l_sendBuf) < len*2+30)
    return 0;
  
  memset(l_sendBuf, 0, sizeof(l_sendBuf));
  count = snprintf(l_sendBuf, sizeof(l_sendBuf), 
                   "AT+NSOST=0,%d.%d.%d.%d,%d,%d,", 
                   l_nbModuConfig.ip[0], l_nbModuConfig.ip[1], l_nbModuConfig.ip[2], 
                   l_nbModuConfig.ip[3], l_nbModuConfig.port, len+sizeof(l_IMEI));
  
  for(int i=0; i<sizeof(l_IMEI); i++)
  {
    count += snprintf(l_sendBuf+count, sizeof(l_sendBuf)-count, "%02X", l_IMEI[i]);
  }
  
  for(int i=0; i<len; i++)
  {
    count += snprintf(l_sendBuf+count, sizeof(l_sendBuf)-count, "%02X", data[i]);
  }
  count += snprintf(l_sendBuf+count, sizeof(l_sendBuf)-count, "\r\n");
  
//  printf(l_sendBuf);
  
  usart_write(BC95COM, l_sendBuf, strlen(l_sendBuf));
  
  return 0;
}

//开机模式选择
static ModeToRun start_mode()
{
  ModeToRun mode = lucTrans;
  uint8_t buf[512] = "";
  int len = 0;
  
  do
  {
    len = usart_read(USERCOM, buf, sizeof(buf), 100);
  }while(len == 0);
  
  if(memmem(buf, len, ATDEBUG, strlen(ATDEBUG)) != NULL)
  {
    mode = atDebug;
    usart_write(USERCOM, ATDEBUG, strlen(ATDEBUG));
  }
  else if(memmem(buf, len, GEMHOCFG, strlen(GEMHOCFG)) != NULL)
  {
    mode = gemhoConfig;
    usart_write(USERCOM, GEMHOCFG, strlen(GEMHOCFG));
  }
  else
  {
    mode = lucTrans;
    
//    if(l_nbModuConfig.sendMode == 0)
//      coap_msgSend(buf, len);
//    else
//      udp_msgSend(buf, len);
		msg_push(l_hMsgFifo, buf, len);
  }
  
  return mode;
}

//gh配置
void ghConfig()
{
  int ret = 0;
  uint8_t *pStart = NULL;
  uint8_t *pEnd = NULL;
  uint8_t buf[128] = "";
  int len = 0;
  
  
  while(1)
  {
    IWDG_Feed();
    ret = usart_read(USERCOM, buf+len, sizeof(buf)-len, 10);
    len += ret;
    
    if(len == 0)
      continue;
      
    pEnd = memmem(buf, len, ENDFLAG, strlen(ENDFLAG));
    if(pEnd != NULL)
    {
      for(int i=0; i<sizeof(cmdExe)/sizeof(cmdExe[0]); i++)
      {
        pStart = memmem(buf, len, cmdExe[i].cmd, strlen(cmdExe[i].cmd));
        if(pStart == buf)
        {
          cmdExe[i].ce_fun((char *)pStart, pEnd - pStart);
          break;
        }
        else if(sizeof(cmdExe)/sizeof(cmdExe[0]) <= i+1) //未定义命令
        {
          usart_write(USERCOM, UDCMD, strlen(UDCMD));
        }
      }
      
      memset(buf, 0, sizeof(buf));
      len = 0;
    }
    
    if(len >= sizeof(buf))
    {
      memset(buf, 0, sizeof(buf));
      len = 0;
      usart_write(USERCOM, ERRORSTR, strlen(ERRORSTR));
    }
    
  }
}

static int wait_OK(int timeout)
{
  int ret = 0;
  char buf[128] = "";
  
  usart_read(BC95COM, buf, sizeof(buf), timeout);
  
  if(memmem(buf, sizeof(buf), OKSTR, strlen(OKSTR)) != NULL)
  {
    ret = 0;
  }
  else
  {
    ret = -1;
  }

  return ret;
}

static int ATCMD_waitOK(char *cmd, int loopTime, int timeout)
{
  int status = 0;
  
  status = -1;
  for(int i=0; i<loopTime; i++)
  {
    usart_write(BC95COM, cmd, strlen(cmd));
    if(wait_OK(timeout) == 0)
    {
      status = 0;
      break;
    }
  }
  
  return status;
}

//获取IMEI码
static int get_IMEI(char *pIMEI)
{
  int ret = 0;
  uint8_t *pStart = NULL;
  uint8_t *pEnd = NULL;
  static char buf[128] = "";
  
  usart_write(BC95COM, IMEIGET, strlen(IMEIGET));
  
  ret = -1;
  if(usart_read(BC95COM, buf, sizeof(buf), 100) > 0)
  {
    pStart = memmem(buf, sizeof(buf), IMEIRTN, strlen(IMEIRTN));
    pEnd = memmem(buf, sizeof(buf), ENDOK, strlen(ENDOK));
    
    if(pStart != NULL && pEnd != NULL && pEnd-pStart-strlen(IMEIRTN)==15)
    {
      memcpy(pIMEI, pStart+strlen(IMEIRTN), 15);
      ret = 0;
    }
  }
  
  return ret;
}

//获取NBAND码
static int get_NBAND(int *pNand)
{
  int ret = 0;
  uint8_t *pStart = NULL;
  uint8_t *pEnd = NULL;
  char buf[128] = "";
  
  usart_write(BC95COM, NBANDGET, strlen(NBANDGET));
  
  ret = -1;
  if(usart_read(BC95COM, buf, sizeof(buf), 300) > 0)
  {
    pStart = memmem(buf, sizeof(buf), NBANDRTN, strlen(NBANDRTN));
    pEnd = memmem(buf, sizeof(buf), ENDOK, strlen(ENDOK));
    
    if(pStart != NULL && pEnd != NULL && pEnd-pStart-strlen(NBANDRTN)==1)
    {
      *pNand = (pStart+strlen(NBANDRTN))[0]-'0';
      ret = 0;
    }
  }
  
  return ret;
}


//初始载入配置
static int load_config()
{
  int ret = 0;
  
  ret = read_config(&l_nbModuConfig);
  
  if(ret != 0)
  {
    memcpy(&l_nbModuConfig, &l_default_nbMCFG, sizeof(l_nbModuConfig));
  }
  
  return ret;
}



static int bc95_cfgBaudRate(uint32_t old_br, uint32_t new_br)
{
  char buf[128] = "";
  int ret = 0;
  
  usart_configure(BC95COM, old_br, 1, 0);
  
  snprintf(buf, sizeof(buf), "AT+NATSPEED=%d,3,0,2,1\r\n", new_br);
  if(ATCMD_waitOK(buf, 60, 30) != 0)
  {
    ret = -1;
  }
  else
  {
    usart_configure(BC95COM, new_br, 1, 0);
    if(ATCMD_waitOK(ATSTR, 3, 30) != 0)
    {
      ret = -1;
    }
  }
  
  return ret;
  
}


//初始配置
static int config_bc95()
{
  int status = 0;
  char buf[128] = "";
	
	//reset bc95
  GPIO_ResetBits(GPIOA, GPIO_Pin_11);
  delay_ms(110);
  GPIO_SetBits(GPIOA, GPIO_Pin_11);
  
  status = ATCMD_waitOK(ATSTR, 60, 100);
  
  status |= bc95_cfgBaudRate(BC95ORGBAUDRATE, l_nbModuConfig.baudrate);
  
  status |= get_IMEI(l_IMEI);
  
  status |= get_NBAND(&l_nband);
  
  
  if(status == 0)
  {
    if(l_nbModuConfig.sendMode == 1) //udp
    {
      snprintf(buf, sizeof(buf), "AT+NSOCR=DGRAM,17,8888,1\r\n");
      if(ATCMD_waitOK(buf, 3, 100) != 0)
      {
        usart_write(USERCOM, "UDP CONFIG FAIL\r\n", strlen("UDP CONFIG FAIL\r\n"));
        status = -2;
      }
    }
    else
    {
      snprintf(buf, sizeof(buf), "AT+NCDP=%d.%d.%d.%d,%d\r\n", 
               l_nbModuConfig.ip[0], l_nbModuConfig.ip[1], l_nbModuConfig.ip[2], 
               l_nbModuConfig.ip[3], l_nbModuConfig.port);
      if(ATCMD_waitOK(buf, 3, 100) != 0)
      {
        usart_write(USERCOM, "COAP CONFIG FAIL\r\n", strlen("COAP CONFIG FAIL\r\n"));
        status = -3;
      }
    }
  }
  else
  {
    usart_write(USERCOM, "INIT FAIL\r\n", strlen("INIT FAIL\r\n"));
  }
  
  if(status == 0)
  {
    usart_write(USERCOM, "INIT OK\r\n", strlen("INIT OK\r\n"));

  }

  return status;
}


static void thread_msgSend(void *args)
{
	while(1)
	{
		char msgData[512] = "";
		uint16_t msgSize = 0;
		int ret = 0;
		ret = msg_pop(l_hMsgFifo, &msgData, &msgSize, 30);
		if(ret == 0)
		{
			if(msgSize > 0)
			{
				usart_write(USERCOM, msgData, msgSize);
			}
		}
		else if(ret == -RT_ETIMEOUT)
		{
			
		}
		
	}
}

static rt_err_t urx_input(rt_device_t dev, rt_size_t size)
{
	
	rt_sem_release(sem_urx);
	
	return RT_EOK;
} 


static void main_entry(void *args)
{
  RCC_Configuration();
  GPIO_Configuration();
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_3);
  tick_ms_init();
  RST_Configuration();
  config_init();
  
  IWDG_Init(5, 0xfff);
  
  //led light off
  GPIO_SetBits(GPIOB, GPIO_Pin_14);
  GPIO_SetBits(GPIOB, GPIO_Pin_15);
  
  //初始配置
  
  load_config();
  
  BC95COM = usart_init("uart1", BC95ORGBAUDRATE, 1, 0);
  USERCOM = usart_init("uart3", l_nbModuConfig.baudrate, l_nbModuConfig.stopbit, l_nbModuConfig.parity);
  
  config_bc95();
	
	l_hMsgFifo = msg_init(COAP_MAXLEN, 5);
  
  ModeToRun mode = start_mode();
	
  
  if(mode == atDebug)
  {
    USERCOM_direct_BC95COM();
  }
  else if(mode == gemhoConfig)
  {
    ghConfig();
  }

	rt_thread_t ht_msgSend = rt_thread_create("thread_msgSend", thread_msgSend, RT_NULL, 1024, 3, 10);
	if (ht_msgSend!= RT_NULL)
		rt_thread_startup(ht_msgSend);
	
	sem_urx = rt_sem_create("sem_urx", 0, RT_IPC_FLAG_PRIO);
	rt_device_set_rx_indicate(USERCOM, urx_input);
	
	while(1)
	{
		if(rt_sem_take(sem_urx, 200) == RT_EOK)
		{
			int len;
			char buf[512] = "";
			
			len = usart_read(USERCOM, buf, sizeof(buf), 100);
			if(len > 0)
			{
				msg_push(l_hMsgFifo, buf, len);
			}
		}
	}
}

int main(void)
{
	rt_thread_t thread = rt_thread_create("main_entry", main_entry, RT_NULL, 2048, 2, 10);
	if (thread!= RT_NULL)
		rt_thread_startup(thread);
	
	while(1)
	{
		rt_thread_delay(20);
	}
}


void EXTI15_10_IRQHandler(void)
{
  if(EXTI_GetITStatus(EXTI_Line13) != RESET)
  {
    int count = 16;
    if(save_config(&l_default_nbMCFG) == 0)
    {
      while(count--)
      {
        GPIO_ResetBits(GPIOB, GPIO_Pin_15);
        delay_ms(100);
        GPIO_SetBits(GPIOB, GPIO_Pin_15);
        delay_ms(100);
      }
    }
    
    /* Clear the  EXTI line 0 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line13);
  }

}




