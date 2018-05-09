#include "time_utils.h"
#include <stdio.h>
#include "stm32f10x_conf.h"
#include "stm32f10x.h"



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








