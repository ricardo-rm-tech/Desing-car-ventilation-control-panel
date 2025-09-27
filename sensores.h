#ifndef __SENSORES_H
#define __SENSORES_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>
#include "Driver_I2C.h"

// Registro del LM75
#define LM75_TEMP_REG      0x00

// Direcciones I2C
#define LM75_EXT_ADDR      0x49   // Sensor exterior
#define LM75_INT_ADDR      0x48   // Sensor interior

#define MSGQUEUE_SENS_OBJECTS  1

typedef struct {
  float Te;   // Temperatura exterior
  float Ti;   // Temperatura interior
} MSGQUEUE_SENS_t;

extern osMessageQueueId_t sens_Queue;

int Init_Thsensor (void);

#endif