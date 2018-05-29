#include "time_utils.h"
#include <stdio.h>
#include "stm32f10x.h"
#include "stm32f10x_iwdg.h"

//#define ENABLEIWDG

static volatile uint64_t l_timestamp = 0;


//��ȡʱ�������λms
uint64_t get_timestamp()
{
  return l_timestamp;
}

void TIM3_Int_Init(u16 arr,u16 psc)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //ʱ��ʹ��
  
  //��ʱ��TIM3��ʼ��
  TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ   
  TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
  
  TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //ʹ��ָ����TIM3�ж�,��������ж�
  
  //�ж����ȼ�NVIC����
  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3�ж�
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //��ռ���ȼ�0��
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  //�����ȼ�3��
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
  NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���
  
  
  TIM_Cmd(TIM3, ENABLE);  //ʹ��TIMx                     
}

//��ʱ��3�жϷ������
void TIM3_IRQHandler(void)   //TIM3�ж�
{
  if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //���TIM3�����жϷ������
  {
    TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //���TIMx�����жϱ�־ 
    l_timestamp++;
  }
}

//systick��ʼ��
void tick_ms_init()
{
  TIM3_Int_Init(9,7199);
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





