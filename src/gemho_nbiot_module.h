#ifndef __GEMHO_CMD_H
#define __GEMHO_CMD_H

#ifdef __cplusplus
 extern "C" {
#endif 

#include <stdint.h>   

   
#pragma pack(1)

typedef struct tag_nbModu_config
{
  uint8_t ip[4];   ///< Source IP Address
  uint16_t port;
  uint8_t sendMode;
  uint32_t baudrate;
  uint8_t stopbit;
  uint8_t parity;
}nbModu_config;

typedef struct tag_confSaveUnit
{
  nbModu_config config;
  uint32_t crc32;
}confSaveUnit;

#pragma pack()


typedef enum tag_ModeToRun{
  atDebug = 0,
  gemhoConfig,
  lucTrans,
}ModeToRun; 


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


#define COAP_MAXLEN (490)
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
#define ENDOK "\r\n\r\nOK"

#define ATIPPORT "AT+IPPORT" //设置ip和port   
#define ATIPPORTEQ "AT+IPPORT=" 
#define ATRS232 "AT+RS232"
#define ATRS232EQ "AT+RS232="
#define ATSEMO "AT+SEMO"    //发送模式选择  
#define ATSEMOEQ "AT+SEMO=" 
#define ATIMEIBD "AT+IMEIBD"//获取imei和nband
#define ATSAVE "AT+SAVE"   //存储配置
#define ATDELO "AT+DELO"  //还原默认配置
#define UDCMD "UNDEFINED CMD\r\n"
   
   
#ifdef __cplusplus
 }
#endif

#endif /* __GEMHO_CMD_H */



