#include "time_utils.h"
#include <stdio.h>
#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#include "stm32f10x_iwdg.h"

//#define ENABLEIWDG

static volatile uint64_t l_timestamp = 0;


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


void delay_ms(uint64_t time_ms)
{
  volatile uint64_t old = get_timestamp();
  
  while(get_timestamp()-old < time_ms);
}


void loop_ms(uint64_t time)
{    
   u16 i=0;  
   while(time--)
   {
      i=8000;  //自己定义
      while(i--) ;    
   }
}


/**
 * 初始化独立看门狗
 * prer:分频数:0~7(只有低 3 位有效!)
 * 分频因子=4*2^prer.但最大值只能是 256!
 * rlr:重装载寄存器值:低 11 位有效.
 * 时间计算(大概):Tout=((4*2^prer)*rlr)/40 (ms).
 */
void IWDG_Init(uint8_t prer,uint16_t rlr)
{
#ifdef ENABLEIWDG
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); /* 使能对寄存器IWDG_PR和IWDG_RLR的写操作*/
    IWDG_SetPrescaler(prer);    /*设置IWDG预分频值:设置IWDG预分频值*/
    IWDG_SetReload(rlr);     /*设置IWDG重装载值*/
    IWDG_ReloadCounter();    /*按照IWDG重装载寄存器的值重装载IWDG计数器*/
    IWDG_Enable();        /*使能IWDG*/
#endif
}


/**
 * 喂独立看门狗
 */
void IWDG_Feed(void)
{
#ifdef ENABLEIWDG
    IWDG_ReloadCounter();    /*reload*/
#endif
}





