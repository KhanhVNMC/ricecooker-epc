#ifndef RICECOOKER_USBD_CONF_H
#define RICECOOKER_USBD_CONF_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>

#define USBD_MAX_NUM_INTERFACES        1u
#define USBD_MAX_NUM_CONFIGURATION     1u
#define USBD_MAX_STR_DESC_SIZ          64u
#define USBD_SELF_POWERED              1u
#define USBD_MAX_POWER                 0x32u // 100mA
#define USBD_LPM_ENABLED               0u
#define USBD_DEBUG_LEVEL               0u // no debug
#define USBD_SUPPORT_USER_STRING_DESC  0u
#define USBD_CLASS_USER_STRING_DESC    0u
#define USBD_CLASS_BOS_ENABLED         0u

// DUMMY static malloc/free for CDC middleware shit, idc
// this is NOT a real heap allocator, free dont do anything
void* usbd_static_mem_dmalloc(uint32_t size);
void  usbd_static_mem_dfree(void*);

// USBD needed macros
#define USBD_malloc     usbd_static_mem_dmalloc
#define USBD_free       usbd_static_mem_dfree
#define USBD_memset     memset
#define USBD_memcpy     memcpy
#define USBD_Delay      HAL_Delay

#define USBD_UsrLog(...) do {} while (0)
#define USBD_ErrLog(...) do {} while (0)
#define USBD_DbgLog(...) do {} while (0)

#endif
