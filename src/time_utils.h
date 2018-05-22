#ifndef __TIME_UTILS_H
#define __TIME_UTILS_H

#ifdef __cplusplus
 extern "C" {
#endif 
   
#include <stdint.h>


//��ȡʱ���
uint64_t get_timestamp();

//systick��ʼ��
void tick_ms_init();

//systick�ж�
void SysTick_Handler(void);

//��ʱ��������λms��
void delay_ms(uint64_t time_ms);

//��ʱ��������λms��
void loop_ms(uint64_t time);


/**
 * ��ʼ���������Ź�
 * prer:��Ƶ��:0~7(ֻ�е� 3 λ��Ч!)
 * ��Ƶ����=4*2^prer.�����ֵֻ���� 256!
 * rlr:��װ�ؼĴ���ֵ:�� 11 λ��Ч.
 * ʱ�����(���):Tout=((4*2^prer)*rlr)/40 (ms).
 */
void IWDG_Init(uint8_t prer,uint16_t rlr);

/**
 * ι�������Ź�
 */
void IWDG_Feed(void);



#ifdef __cplusplus
}
#endif

#endif /* __TIME_UTILS_H */


