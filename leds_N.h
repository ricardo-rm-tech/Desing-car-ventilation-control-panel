#ifndef __leds_N_H
#define __leds_N_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>


#define LedVerde_ON  0x001
#define LedVerde_OFF 0x002

#define LedAzul_ON  0x004
#define LedAzul_OFF 0x008

#define LedRojo_ON  0x010
#define LedRojo_OFF 0x020

int Init_ThLEDS (void);                     // Funcion de creacion e inicializacion del thread asociado a los LEDS

#endif
