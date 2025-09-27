#include "leds_N.h"
 
/*----------------------------------------------------------------------------
 
Thread 1 'LEDS': Sample thread
---------------------------------------------------------------------------*/

osThreadId_t tid_Control_Led;        // Thread Leds
void ThControlLed (void* argument);                  // Thread function led 1


const osThreadAttr_t thread1_attr_leds = {
  .stack_size = 128                            // Create the thread stack with a size of 128 bytes
};


int Init_ThLEDS (void)
{
	
	tid_Control_Led = osThreadNew(ThControlLed, NULL, &thread1_attr_leds);
	
  if (tid_Control_Led == NULL)
  {
    return(-1);
  }
	
  return(0);
}


void ThControlLed(void* args){

	uint32_t status;
	
  while(1){
		status = osThreadFlagsWait(LedVerde_ON | LedAzul_ON | LedRojo_ON, osFlagsWaitAny, osWaitForever);
		
		if (status & LedVerde_ON) {
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
    }
    /*if (status & LedVerde_OFF) {
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
    }*/
    if (status & LedAzul_ON) {
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
    }
    /*if (status & LedAzul_OFF) {
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_RESET);
    }*/
    if (status & LedRojo_ON) {
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_SET);
    }
    /*if (status & LedRojo_OFF) {
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
    }*/
		
		
  }
}
