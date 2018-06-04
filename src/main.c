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
//static char l_sendBuf[2048] = "";
//static uint32_t l_cnt = 0;


static void *USERCOM = NULL;
static void *BC95COM = NULL;
static void *l_hMsgFifo = NULL;
static rt_sem_t sem_urx = RT_NULL;
static DeviceStatus l_devStatus = DEVICEOK;

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

static int wait_OK_timeout(int timeout)
{
  int ret = 0;
  char buf[128] = "";
	int cnt = 0;
	int len = 0;
	uint64_t LastTime = get_timestamp();
	
	while(get_timestamp() - LastTime <= timeout)
	{
		if(cnt > sizeof(buf))
			cnt = sizeof(buf);
		
		len = usart_read(BC95COM, buf+cnt, sizeof(buf)+cnt, 20);
		if(len > 0)
		{
			cnt += len;
		}
  
		if(memmem(buf, sizeof(buf), OKSTR, strlen(OKSTR)) != NULL)
		{
			ret = 0;
			break;
		}
		else
		{
			ret = -1;
		}
	}

  return ret;
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


//coap发送
static int coap_msgSend(uint8_t *data, uint32_t len, uint32_t repeats)
{
  char buf[128] = "";
	uint32_t count = 0;
	int ret = -1;
  
  if(len > COAP_MAXLEN)
    len = COAP_MAXLEN;
  
	for(int i=0; i<repeats; i++)
	{
		count = snprintf(buf, sizeof(buf), "AT+NMGS=%d,00%04X", len+3, len);
		usart_write(BC95COM, buf, count);

		for(int i=0; i<len; i++)
		{
			count = snprintf(buf, sizeof(buf), "%02X", data[i]);
			usart_write(BC95COM, buf, count);
		}
		count = snprintf(buf, sizeof(buf), "\r\n");
		usart_write(BC95COM, buf, count);
		
		if(wait_OK_timeout(2000) == 0)
		{
			ret = 0;
			break;
		}
	}

  return ret;
}


//udp发送
static int udp_msgSend(uint8_t *data, uint32_t len, uint32_t repeats)
{
  char buf[128] = "";
	uint32_t count = 0;
	int ret = -1;
  
  if(len > COAP_MAXLEN)
    len = COAP_MAXLEN;
	
	for(int i=0; i<repeats; i++)
	{
  
		count = snprintf(buf, sizeof(buf), 
										 "AT+NSOST=0,%d.%d.%d.%d,%d,%d,", 
										 l_nbModuConfig.ip[0], l_nbModuConfig.ip[1], l_nbModuConfig.ip[2], 
										 l_nbModuConfig.ip[3], l_nbModuConfig.port, len+sizeof(l_IMEI));
		usart_write(BC95COM, buf, count);
		
		for(int i=0; i<sizeof(l_IMEI); i++)
		{
			count = snprintf(buf, sizeof(buf), "%02X", l_IMEI[i]);
			usart_write(BC95COM, buf, count);
		}
		
		for(int i=0; i<len; i++)
		{
			count = snprintf(buf, sizeof(buf), "%02X", data[i]);
			usart_write(BC95COM, buf, count);
		}
		count = snprintf(buf, sizeof(buf), "\r\n");
		usart_write(BC95COM, buf, count);
		
		if(wait_OK_timeout(2000) == 0)
		{
			ret = 0;
			break;
		}
	}
  
  return ret;
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
	
	ret = -1;
	
	for(int i=0; i<3; i++)
	{
		usart_write(BC95COM, IMEIGET, strlen(IMEIGET));
		
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
  
  
  
  ret = -1;
	for(int i=0; i<3; i++)
	{
		usart_write(BC95COM, NBANDGET, strlen(NBANDGET));
		if(usart_read(BC95COM, buf, sizeof(buf), 300) > 0)
		{
			pStart = memmem(buf, sizeof(buf), NBANDRTN, strlen(NBANDRTN));
			pEnd = memmem(buf, sizeof(buf), ENDOK, strlen(ENDOK));
			
			if(pStart != NULL && pEnd != NULL && pEnd-pStart-strlen(NBANDRTN)==1)
			{
				*pNand = (pStart+strlen(NBANDRTN))[0]-'0';
				ret = 0;
				break;
			}
		}
	}
  
  return ret;
}

//查询是否附着网络
static int get_CGATT(int *pCGATT)
{
  int ret = 0;
  uint8_t *pStart = NULL;
  uint8_t *pEnd = NULL;
  char buf[128] = "";

  ret = -1;
	for(int i=0; i<3; i++)
	{
		usart_write(BC95COM, CGATTGET, strlen(CGATTGET));
		if(usart_read(BC95COM, buf, sizeof(buf), 300) > 0)
		{
			pStart = memmem(buf, sizeof(buf), CGATTRTN, strlen(CGATTRTN));
			pEnd = memmem(buf, sizeof(buf), ENDOK, strlen(ENDOK));
			
			if(pStart != NULL && pEnd != NULL && pEnd-pStart-strlen(CGATTRTN)==1)
			{
				*pCGATT = (pStart+strlen(CGATTRTN))[0]-'0';
				ret = 0;
				break;
			}
		}
	}
  
  return ret;
}

//设置附着网络
static int set_CGATT(int CGATT)
{
  int ret = 0;
  char buf[128] = "";

  ret = -1;
	for(int i=0; i<3; i++)
	{
		snprintf(buf, sizeof(buf), "%s%d\r\n", CGATTSET, CGATT);
		usart_write(BC95COM, buf, strlen(buf));
		if(wait_OK(100) == 0)
		{
			ret = 0;
			break;
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
	
	usart_configure(BC95COM, BC95ORGBAUDRATE, 1, 0);
	
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

static int setDevicStatus(DeviceStatus status)
{
	l_devStatus = status;
	
	return 0;
}

static DeviceStatus getDevicStatus()
{
	return l_devStatus;
}

//bc95状态判断和恢复，如果附着失败就重启bc95重新附着。
static DeviceStatus BC95_statusManage(int CGATTLoops, rt_tick_t interval, uint64_t CGATT_timeout)
{
	DeviceStatus status = DEVICEOK;
	int bc95ReConfig = 0;
	
	for(int i=0; i<CGATTLoops; i++)
	{
		int ret = 0;
		int nCGATT = 0;

		//重启配置bc95
		if(bc95ReConfig == 1)
		{
			ret = config_bc95();
			if(ret != 0)
			{
				status = BC95DONTWORK;
				break;
			}
			else
				continue;
		}

		bc95ReConfig = 0;
		ret	= get_CGATT(&nCGATT);
		
		//无法获取CGATT就重启配置bc95
		if(ret != 0)
		{
			bc95ReConfig = 1;
			continue;
		}
		
		//判断附着状态
		if(nCGATT == 1)
		{
			status = DEVICEOK;
			break;
		}
		else
		{
			uint64_t LastTime = get_timestamp();
			status = CGATTTIMEOUT;
			set_CGATT(1);
			while(get_timestamp() <= LastTime+CGATT_timeout)
			{
				ret = get_CGATT(&nCGATT);
				if(ret != 0)
				{
					status = BC95DONTWORK;
					bc95ReConfig = 1;
					break;
				}
				else
				{
					if(nCGATT == 1)
					{
						status = DEVICEOK;
						break;
					}
					else
						rt_thread_delay(interval);
				}
			}
			
			//如果超时就重启重新配置bc95
			if(status == CGATTTIMEOUT)
				bc95ReConfig = 1;
		}
		
	}
	
	return status;
}


static void thread_msgSend(void *args)
{
	while(1)
	{
		uint8_t msgData[512] = "";
		uint16_t msgSize = 0;
		int ret = 0;
		int repaets = 3;
		uint32_t failTimes = 0;
		DeviceStatus devStatus = DEVICEOK;
		uint32_t popWait = 3;
		
		ret = msg_pop(l_hMsgFifo, &msgData, &msgSize, rt_tick_from_millisecond(popWait*1000));
		if(ret == 0)
		{
			if(msgSize > 0)
			{
				failTimes = 0;
				
				while(1)
				{
					if(l_nbModuConfig.sendMode == 0)
						ret = coap_msgSend(msgData, msgSize, repaets);
					else
						ret = udp_msgSend(msgData, msgSize, repaets);
				
					if(ret != 0)//发送失败进入状态判断和恢复。
					{
						failTimes++;//失败次数决定状态判断和恢复的间隔  间隔为failTimes*10分钟
//						devStatus = BC95_statusManage(3, rt_tick_from_millisecond(1*1000), 10*1000);
						devStatus = BC95_statusManage(3, rt_tick_from_millisecond(1*1000), 2*60*1000); //入网判断时间2分钟
						setDevicStatus(devStatus);
						if(devStatus != DEVICEOK)
						{
//							rt_thread_delay(rt_tick_from_millisecond(failTimes*1000));
							rt_thread_delay(rt_tick_from_millisecond(failTimes*10*60*1000));
						}
					}
					else
					{
						failTimes = 0;
						break;
					}
				}

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
  
  if(config_bc95() != 0)
	{
		setDevicStatus(BC95DONTWORK);
	}
	
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

	rt_thread_t ht_msgSend = rt_thread_create("thread_msgSend", thread_msgSend, RT_NULL, 1024+512, 3, 10);
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

static void status_entry(void *args)
{
	while(1)
	{
		DeviceStatus status = getDevicStatus();
		int blinkInter = 200;
		
		if(status == DEVICEOK)
		{
			GPIO_SetBits(GPIOB, GPIO_Pin_14);
		}
		else if(status == BC95DONTWORK)
		{
			for(int i=0; i<2; i++)
			{
				GPIO_ResetBits(GPIOB, GPIO_Pin_14);
				rt_thread_delay(rt_tick_from_millisecond(blinkInter));
				GPIO_SetBits(GPIOB, GPIO_Pin_14);
				rt_thread_delay(rt_tick_from_millisecond(blinkInter));
			}
		}
		else if(status == CGATTTIMEOUT)
		{
			for(int i=0; i<3; i++)
			{
				GPIO_ResetBits(GPIOB, GPIO_Pin_14);
				rt_thread_delay(rt_tick_from_millisecond(blinkInter));
				GPIO_SetBits(GPIOB, GPIO_Pin_14);
				rt_thread_delay(rt_tick_from_millisecond(blinkInter));
			}
		}
		
		rt_thread_delay(rt_tick_from_millisecond(1500));
	}
		
}

int main(void)
{
	rt_thread_t ht_main = rt_thread_create("main_entry", main_entry, RT_NULL, 2048, 2, 10);
	if (ht_main!= RT_NULL)
		rt_thread_startup(ht_main);
	
	rt_thread_t ht_status = rt_thread_create("status_entry", status_entry, RT_NULL, 256, 6, 10);
	if (ht_status!= RT_NULL)
		rt_thread_startup(ht_status);
	
	while(1)
	{
		rt_thread_delay(rt_tick_from_millisecond(2*1000));
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




