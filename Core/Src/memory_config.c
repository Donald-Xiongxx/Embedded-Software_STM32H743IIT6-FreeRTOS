#include "FreeRTOS.h"
#include <stdint.h>

#if ( configAPPLICATION_ALLOCATED_HEAP == 1 )

__attribute__((section(".ucHeap"))) uint8_t ucHeap[ configTOTAL_HEAP_SIZE ];

#endif
