#include "ThClk.h"
 
/*----------------------------------------------------------------------------
 
Thread 1 'Clock': Sample thread
---------------------------------------------------------------------------*/

osThreadId_t tid_Control_reloj;        // Thread reloj

uint8_t Segundos;
uint8_t Minutos;
uint8_t Horas;

void ThControlReloj (void* argument);                   // Thread function

const osThreadAttr_t thread1_attr_clk = {
  .stack_size = 256                            // Create the thread stack with a size of 512 bytes
};

int Init_ThClk (void)
{
	
  tid_Control_reloj = osThreadNew(ThControlReloj, NULL, &thread1_attr_clk);
		
  if (tid_Control_reloj == NULL)
  {
    return(-1);
  }
  return(0);
}

void ThControlReloj(void* args){
  Segundos = 0;
  Minutos = 0;
  Horas=0;
	
  while(1){

		Segundos++;
   if (Segundos == 60){
     Segundos = 0;
     Minutos++;
   }
   if (Minutos == 60){
     Minutos = 0;
     Horas++;
   }
	 if (Horas == 24){
		Horas=0;
	 }
	 
	 
  osDelay(1000);
  }


}
