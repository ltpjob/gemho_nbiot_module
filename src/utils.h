#ifndef __UTILS_H
#define __UTILS_H

#ifdef __cplusplus
 extern "C" {
#endif 
   
#include <stdio.h>

 
void *memmem(const void *l, size_t l_len, const void *s, size_t s_len);
   
int checkConfigIPORT(char *pIPort, int *pOutIp, int *pOutPort);
   
#ifdef __cplusplus
 }
#endif

#endif /* __UTILS_H */