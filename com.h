#ifndef __COM_H
#define __COM_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "Driver_USART.h"

int Init_ThCom(void);                                                  // Funcion de creacion e inicializacion del thread asociado al CLK

#define SOH 0x01
#define EOT 0xFE
#define Flag_Recibido 0x01
#define Flag_Transmitido 0x01
#define BUFFER_SIZE 64

typedef enum{
	InitState,
	DefaultState
	
}Estado_t;


#endif

