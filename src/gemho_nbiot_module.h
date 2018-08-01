#ifndef __GEMHO_CMD_H
#define __GEMHO_CMD_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include <stdint.h>   

	 
#define MSG_MAXLEN (490)
   
#pragma pack(1)

typedef struct tag_nbModu_config
{
  uint8_t ip[4];   ///< Source IP Address
  uint16_t port;
  uint8_t sendMode;
  uint32_t baudrate;
  uint8_t stopbit;
  uint8_t parity;
	uint8_t watchdog;
	uint8_t msgSave;
}nbModu_config;

typedef struct tag_confSaveUnit
{
  nbModu_config config;
  uint32_t crc32;
}confSaveUnit;

#pragma pack()

typedef struct tag_lightAction
{
  uint32_t interval_time;
  uint32_t blink_times;
	uint32_t stop_time;
}lightAction;

typedef enum tag_ModeToRun{
  atDebug = 0,
  gemhoConfig,
  lucTrans,
}ModeToRun; 

typedef enum tag_DeviceStatus{
	DEVICEOK = 0,
  BC95DONTWORK = -1,
	CGATTTIMEOUT = -2,
	CGATTTGETFAIL = -3,
}DeviceStatus;


typedef int (*ghCmd_excute)(char *, int);

typedef struct tag_cmdExcute
{
  char *cmd;
  ghCmd_excute ce_fun;
}cmdExcute;


//#define USERCOM USART3
//#define BC95COM USART1
#define UCOMBAUDRATE 115200
#define BC95ORGBAUDRATE 9600



#define ATDEBUG "MODE+ATDEBUG\r\n"
#define GEMHOCFG "MODE+GEMHOCFG\r\n"
#define OKSTR "OK\r\n"
#define ERRORSTR "ERROR\r\n"
#define ENDFLAG "\r\n"
#define ATSTR "AT\r\n"
#define IMEIGET "AT+CGSN=1\r\n"
#define IMEIRTN "+CGSN:"
#define NBANDGET "AT+NBAND?\r\n"
#define NBANDRTN "+NBAND:"
#define CGATTGET "AT+CGATT?\r\n"
#define CGATTSET "AT+CGATT="
#define CGATTRTN "+CGATT:"
#define ENDOK "\r\n\r\nOK"

#define ATCIMI "AT+CIMI" //��ȡIMSI
#define ATCSQ "AT+CSQ" //��ȡ�ź�ǿ��
#define ATNUESTATS "AT+NUESTATS" //��ȡģ��״̬
#define ATCGATT "AT+CGATT" //���總�����
#define ATVER "AT+VER" //��ȡ�汾��

#define ATMSGS "AT+MSGS" //���û����С   
#define ATMSGSEQ "AT+MSGS=" 
#define ATIPPORT "AT+IPPORT" //����ip��port   
#define ATIPPORTEQ "AT+IPPORT=" 
#define ATRS232 "AT+RS232"
#define ATRS232EQ "AT+RS232="
#define ATSEMO "AT+SEMO"    //����ģʽѡ��  
#define ATSEMOEQ "AT+SEMO=" 
#define ATWDT "AT+WDT"    //�Ƿ������Ź�  
#define ATWDTEQ "AT+WDT=" 
#define ATIMEIBD "AT+IMEIBD"//��ȡimei��nband
#define ATSAVE "AT+SAVE"   //�洢����
#define ATDELO "AT+DELO"  //��ԭĬ������
#define UDCMD "UNDEFINED CMD\r\n"
#define KPMFT "key push message for test"

#define VERSION "1.0.6a84ddb.1"
   
#ifdef __cplusplus
 }
#endif

#endif /* __GEMHO_CMD_H */



