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



#ifdef __cplusplus
}
#endif

#endif /* __TIME_UTILS_H */


