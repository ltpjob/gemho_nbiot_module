#ifndef __CONFIG_UTILS_H
#define __CONFIG_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif 
  
#include "gemho_nbiot_module.h"
  
void config_init(void);

int save_config(const nbModu_config *pConfig);

int read_config(nbModu_config *pConfig);

void config_init(void);
  
#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_UTILS_H */


