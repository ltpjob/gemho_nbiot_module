#include "utils.h"
#include <stdio.h>
#include "stm32f10x_conf.h"
#include "stm32f10x.h"
#include <string.h>

void *memmem(const void *l, size_t l_len, const void *s, size_t s_len)  
{  
    register char *cur, *last;  
    const char *cl = (const char *)l;  
    const char *cs = (const char *)s;  
   
    /* we need something to compare */  
    if (l_len == 0 || s_len == 0)  
        return NULL;  
   
    /* "s" must be smaller or equal to "l" */  
    if (l_len < s_len)  
        return NULL;  
   
    /* special case where s_len == 1 */  
    if (s_len == 1)  
        return memchr(l, (int)*cs, l_len);  
   
    /* the last position where its possible to find "s" in "l" */  
    last = (char *)cl + l_len - s_len;  
   
    for (cur = (char *)cl; cur <= last; cur++)  
        if (cur[0] == cs[0] && memcmp(cur, cs, s_len) == 0)  
            return cur;  
   
    return NULL;  
}  

int checkConfigIPORT(char *pIPort, int *pOutIp, int *pOutPort)
{
  int checkFlag = 0;
  
  if (sscanf(pIPort, "%d.%d.%d.%d,%d",
             &pOutIp[0], &pOutIp[1], &pOutIp[2], &pOutIp[3], pOutPort) == 5)
  {
    for(int i=0; i<4; i++)
    {
      if(pOutIp[i] > 255 || pOutIp[i] < 0)
      {
        checkFlag = -1;
        break;
      }   
    }
    
    if(*pOutPort<0 || *pOutPort>65535)
      checkFlag = -1;
  }
  else
    checkFlag = -1;
  
  return checkFlag;
}

