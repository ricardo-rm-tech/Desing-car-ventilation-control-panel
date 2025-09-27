#ifndef __Pot_H
#define __Pot_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define POT_MIN_TEMP     2.0f
#define POT_MAX_TEMP    30.0f
#define POT_ADC_MAX   4095.0f

#define MSGQUEUE_POT_OBJECTS  1

typedef struct {
  float Te;   /* Temperatura exterior simulada   */
  float Ta;   /* Temperatura de alarma simulada  */
} MSGQUEUE_POT_t;

extern osMessageQueueId_t pot_Queue;         

int Init_ThPot (void); 

#endif /* __POTENCIOMETRO_H */