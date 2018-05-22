#include "time_utils.h"
#include <stdio.h>
#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#include "stm32f10x_iwdg.h"

//#define ENABLEIWDG

static volatile uint64_t l_timestamp = 0;


//��ȡʱ�������λms
uint64_t get_timestamp()
{
  return l_timestamp;
}

//systick��ʼ��
void tick_ms_init()
{
  if (SysTick_Config(SystemCoreClock / 1000))
  {
    /* Capture error */ 
    while (1);
  }
}

//systick�ж�
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
      i=8000;  //�Լ�����
      while(i--) ;    
   }
}


/**
 * ��ʼ���������Ź�
 * prer:��Ƶ��:0~7(ֻ�е� 3 λ��Ч!)
 * ��Ƶ����=4*2^prer.�����ֵֻ���� 256!
 * rlr:��װ�ؼĴ���ֵ:�� 11 λ��Ч.
 * ʱ�����(���):Tout=((4*2^prer)*rlr)/40 (ms).
 */
void IWDG_Init(uint8_t prer,uint16_t rlr)
{
#ifdef ENABLEIWDG
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); /* ʹ�ܶԼĴ���IWDG_PR��IWDG_RLR��д����*/
    IWDG_SetPrescaler(prer);    /*����IWDGԤ��Ƶֵ:����IWDGԤ��Ƶֵ*/
    IWDG_SetReload(rlr);     /*����IWDG��װ��ֵ*/
    IWDG_ReloadCounter();    /*����IWDG��װ�ؼĴ�����ֵ��װ��IWDG������*/
    IWDG_Enable();        /*ʹ��IWDG*/
#endif
}


/**
 * ι�������Ź�
 */
void IWDG_Feed(void)
{
#ifdef ENABLEIWDG
    IWDG_ReloadCounter();    /*reload*/
#endif
}





