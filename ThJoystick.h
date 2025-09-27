#ifndef __THJOYSTICK_H
#define __THJOYSTICK_H

/*  Includes  */
#include "cmsis_os2.h"  
#include "stm32f4xx_hal.h"

/*  Exported constants  */
#define MSGQUEUE_JOYSTICK_OBJECTS     1

#define UP                            GPIO_PIN_10
#define RIGHT                         GPIO_PIN_11
#define DOWN                          GPIO_PIN_12
#define LEFT                          GPIO_PIN_14
#define CENTER                        GPIO_PIN_15
#define PULSACION                     0x10
#define UP_CORTA                      0x01
#define RIGHT_CORTA                   0x02
#define DOWN_CORTA                    0x03
#define LEFT_CORTA                    0x04
#define CENTER_CORTA                  0x05
#define CENTER_LARGA                  0x07

extern osMessageQueueId_t mid_MsgQueue_JOY;
/*  Objeto de tipo MSQQUEUE_JOY_t */
typedef struct {
  uint8_t gesto;
} MSGQUEUE_JOY_t;

/*  Exported functions  */
int Init_ThJoystick (void);                // Inicializacion del modulo JOYSTICK
int Init_Thread_Joystick_Test (void);     // Hilo de Test

#endif