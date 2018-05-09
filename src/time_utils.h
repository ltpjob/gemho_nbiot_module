#ifndef __TIME_UTILS_H
#define __TIME_UTILS_H

#ifdef __cplusplus
 extern "C" {
#endif 
   
#include <stdint.h>


//获取时间戳
uint64_t get_timestamp();

//systick初始化
void tick_ms_init();

//systick中断
void SysTick_Handler(void);



#ifdef __cplusplus
}
#endif

#endif /* __TIME_UTILS_H */


