#ifndef __PWM_H
#define __PWM_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>


#define PWM_TIMER               TIM1 
#define PWM_CHANNEL             TIM_CHANNEL_1
#define PWM_GPIO_PORT           GPIOE
#define PWM_GPIO_PIN            GPIO_PIN_9
#define PWM_GPIO_AF             GPIO_AF1_TIM1

#define PWM_FREQUENCY_HZ        1000U       
#define PWM_DUTY_MIN            0U           
#define PWM_DUTY_MAX            100U        


#define MSGQUEUE_PWM_OBJECTS    1

typedef struct {
  uint8_t duty;   // Ciclo de trabajo 0-100 % 
} MSGQUEUE_PWM_t;

extern osMessageQueueId_t pwm_Queue;     

int Init_ThPWM (void);  

#endif /* __SENAL_PWM_H */