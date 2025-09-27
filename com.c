#include "COM.h"
 
/*----------------------------------------------------------------------------
 
Thread 1 'Acelerometro': Sample thread
---------------------------------------------------------------------------*/

osThreadId_t tid_Control_Com_recepcion;        // Thread Joystickid
osThreadId_t tid_Control_Com_transmision;

osMessageQueueId_t cola_entrada;
osMessageQueueId_t cola_salida;
void ThControlComRecepcion (void* argument);
void ThControlComTransmision (void* argument);

extern ARM_DRIVER_USART Driver_USART3;

void Com_Callback(uint32_t event);

static ARM_DRIVER_USART * USARTdrv = &Driver_USART3;


const osThreadAttr_t thread1_attr_com = {
  .stack_size = 1024                            // Create the thread stack with a size of 512 bytes
};


int Init_ThCom (void)
{
	
  tid_Control_Com_recepcion = osThreadNew(ThControlComRecepcion, NULL, &thread1_attr_com); 
	tid_Control_Com_transmision = osThreadNew(ThControlComTransmision, NULL, &thread1_attr_com);
	
  if (tid_Control_Com_recepcion == NULL)
  {
    return(-1);
  }
	if (tid_Control_Com_transmision == NULL)
  {
    return(-1);
  }
  return(0);
}

void ThControlComRecepcion(void *argument) {

	//busyEventFlag = osEventFlagsNew(NULL);
	Estado_t estado = InitState;
	uint8_t bufferDatos[BUFFER_SIZE];
	int i=1;
	cola_salida = osMessageQueueNew(4, sizeof(bufferDatos), NULL); 
	
	ARM_DRIVER_VERSION version;
	ARM_USART_CAPABILITIES drv_capabilities;
	uint8_t cmd;	
	USARTdrv->Initialize(Com_Callback);
	USARTdrv->PowerControl(ARM_POWER_FULL);
	USARTdrv->Control(ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE | ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE, 115200);
	
	USARTdrv->Control(ARM_USART_CONTROL_TX, 1);
	USARTdrv->Control(ARM_USART_CONTROL_RX, 1);
	
	while (1) {
		
		//USARTdrv->Receive(&cmd, 1);
		if (USARTdrv->Receive(&cmd, 1) != ARM_DRIVER_OK) {
    // Manejar el error
			while(1){}
		}
		osThreadFlagsWait(Flag_Recibido, osFlagsWaitAny, osWaitForever);

		switch(estado){
		
			case InitState:
				
			if(cmd == SOH){
				bufferDatos[0]= 0x01;
				estado = DefaultState;
			}
			
			break;
			
			
			case DefaultState:
				
			if(cmd == EOT){
			
				if(bufferDatos[2] == (i+1)){
					
					bufferDatos[i]=EOT;
					osMessageQueuePut(cola_salida, &bufferDatos, NULL, 0U);
					i=1;
					
				}else{
				 //ERROR
					while(1){
					}
					i=1;
				}
				
				estado = InitState;
				
			}else{
				bufferDatos[i] = cmd;
				i++;
			}
			
			break;
		
		
		}
			
	}
}

void ThControlComTransmision (void* argument){

	uint8_t buffer_datos_entrada[BUFFER_SIZE];
	cola_entrada = osMessageQueueNew(10, sizeof(buffer_datos_entrada), NULL);
	
	int i=0;
	while(1){
		osMessageQueueGet(cola_entrada, &buffer_datos_entrada, NULL, osWaitForever);
		
		do{
			USARTdrv->Send(&buffer_datos_entrada[i], 1);
			i++;
			osThreadFlagsWait(Flag_Transmitido, osFlagsWaitAny, osWaitForever);
		}while(buffer_datos_entrada[i]!= EOT);
		USARTdrv->Send(&buffer_datos_entrada[i], 1);
		osThreadFlagsWait(Flag_Transmitido, osFlagsWaitAny, osWaitForever);
		i=0;
	}

}



void Com_Callback(uint32_t event) {
    // Manejo de eventos del driver
	uint32_t mask;
	uint32_t mask2;
	mask = ARM_USART_EVENT_RECEIVE_COMPLETE;
	mask2= ARM_USART_EVENT_TRANSFER_COMPLETE | ARM_USART_EVENT_SEND_COMPLETE | ARM_USART_EVENT_TX_COMPLETE;
	if(event & mask) {
		osThreadFlagsSet(tid_Control_Com_recepcion, Flag_Recibido);
	}
	if(event & mask2){
		osThreadFlagsSet(tid_Control_Com_transmision, Flag_Transmitido);
	}
	
}