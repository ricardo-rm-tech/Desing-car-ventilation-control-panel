#include "lcd.h"
#include "Arial12x12.h"

/*----------------------------------------------------------------------------
 *      Thread 2 'ThLCD': Sample thread
 *---------------------------------------------------------------------------*/

osThreadId_t tid_ThLCD;                        // thread LCD id

osMessageQueueId_t lcd_Queue;

void ThLCD (void *argument);                   // thread LCD function

/* SPI Driver */
extern ARM_DRIVER_SPI Driver_SPI1;
ARM_DRIVER_SPI* SPIdrv = &Driver_SPI1;

/* Variables LCD */
MSGQUEUE_lcd_t msg_rec_lcd;       // mensaje a recibir

TIM_HandleTypeDef htim7;      // Timer 7
char buffer[512];             // Buffer para poder escribir en el LCD
uint8_t positionL1 = 0;       // contador para mover caracteres en la linea 1
uint16_t positionL2 = 0;      // contador para mover caracteres en la linea 2


osEventFlagsId_t busyEventFlag;
static void busy(uint32_t event);

const osThreadAttr_t thread1_attr_lcd = {
  .stack_size = 512                            // Create the thread stack with a size of 512 bytes
};


/* Funciones LCD */
int Init_ThLCD (void)
{
  tid_ThLCD = osThreadNew(ThLCD, NULL, &thread1_attr_lcd);
		
	lcd_Queue = osMessageQueueNew(1, sizeof(msg_rec_lcd), NULL);  
	
  if (tid_ThLCD == NULL)
  {
    return(-1);
  }	
  return(0);
}

void ThLCD (void *argument)
{
  LCD_Init();
  
  while(1)
  {		
    osMessageQueueGet(lcd_Queue, &msg_rec_lcd, 0U, osWaitForever);     // Se espera a recibir un mensaje
    if(msg_rec_lcd.linea == 1){
			LCD_borrar(1);
			positionL1 = 0;
		}else{
			LCD_borrar(2);
			positionL2 = 0;
		}
		escribir_linea(msg_rec_lcd.linea, msg_rec_lcd.mensaje);
		LCD_update();
  }
}

void LCD_Init(void)
{
  LCD_reset();
  LCD_setup();
  LCD_borrar(1);
	LCD_borrar(2);
  LCD_update();
}

void LCD_reset(void)
{
  static GPIO_InitTypeDef GPIO_InitStruct = {0};
  busyEventFlag = osEventFlagsNew(NULL);
	
  SPIdrv->Initialize(busy);                                                                                      // Initialize the SPI driver
  SPIdrv->PowerControl(ARM_POWER_FULL);                                                                          // Power up the SPI peripheral
  SPIdrv->Control(ARM_SPI_MODE_MASTER | ARM_SPI_CPOL1_CPHA1 | ARM_SPI_MSB_LSB | ARM_SPI_DATA_BITS(8), 20000000); // Configure the SPI to Master, CPOL1 & CPHA1, MSB to LSB, 8 bits, 20 MHz
  
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  
  /* ********* PIN PF13 -> A0 ********* */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_13;    // A0 --> PF13
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
  /* ********************************** */
  
  /* ********* PIN PD14 -> CS ********* */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_14;    // CS --> PD14
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  /* ********************************** */
  
  /* ********* PIN PA6 -> RESET ********* */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitStruct.Pin = GPIO_PIN_6;     // RESET --> PA6
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  /* ********************************** */

  // Seleccionar reset = 0. Activar el reset
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
  delay(10);    // El reset tiene que estar a nivel bajo un tiempo minimo de 1 us (con 10 us tenemos de sobra)
  
  //Seleccionar reset = 1. Desactivar el reset
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
  delay(1000);  // 1 ms de retardo antes de iniciar la secuencia de comandos de inicializacion del LCD
}

void delay(uint32_t n_microsegundos)
{
  htim7.Instance = TIM7;
  
  // Configurar y arrancar el timer para generar un evento pasados n_microsegundos
  __HAL_RCC_TIM7_CLK_ENABLE(); 
  
  htim7.Init.Prescaler = 83;                // 84MHz / 84 = 1 MHz = 1 us
  htim7.Init.Period = n_microsegundos - 1;  // 1 MHz / 1000 = 1000 Hz = 1 ms
                                            // 1 MHz / 10 = 100 kHz = 10 us
  
  HAL_TIM_Base_Init(&htim7);                // CNT = 0
  HAL_TIM_Base_Start(&htim7);               // Se lanza el timer
  
  // Esperar a que se active el flag del registro de Match correspondiente
  while (__HAL_TIM_GET_FLAG(&htim7, TIM_FLAG_UPDATE) == false);
  
  // Borrar el flag
  __HAL_TIM_CLEAR_FLAG(&htim7, TIM_FLAG_UPDATE);
  
  // Parar el Timer y ponerlo a 0 para la siguiente llamada a la función
  HAL_TIM_Base_Stop(&htim7);
}

static void busy(uint32_t event)
{
	uint32_t mask;
	mask = ARM_SPI_EVENT_TRANSFER_COMPLETE; 
	if (event & mask) {
        osEventFlagsSet(busyEventFlag, 0x01); // Establece el flag cuando se completa
  }
}

void LCD_wr_data(unsigned char data)
{
  ARM_SPI_STATUS stat;
  
  // Seleccionar CS = 0
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
  
  // Seleccionar A0 = 1. Para guardar informacion en la memoria del display
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_SET);
  
  // Escribir un dato usando la funcion SPIDrv->Send()
  SPIdrv->Send(&data, sizeof(data));
  
  // Esperar a que se libere el bus SPI
		
		
	osEventFlagsWait(busyEventFlag, 0x01, osFlagsWaitAny, osWaitForever);
  osEventFlagsClear(busyEventFlag, 0x01);
	
  // Seleccionar CS = 1
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
}

void LCD_wr_cmd(unsigned char cmd)
{
  ARM_SPI_STATUS stat;
  
  // Seleccionar CS = 0
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
  
  // Seleccionar A0 = 0. Para interpretar como comandos la informacion recibida
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_13, GPIO_PIN_RESET);
  
  // Escribir un comando (cmd) usando la funcion SPIDrv->Send()
  SPIdrv->Send(&cmd, sizeof(cmd));
  
	
	osEventFlagsWait(busyEventFlag, 0x01, osFlagsWaitAny, osWaitForever); 
  osEventFlagsClear(busyEventFlag, 0x01);
	
  // Seleccionar CS = 1
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
}

void LCD_setup(void)
{
  LCD_wr_cmd(0xAE); // Display OFF
  LCD_wr_cmd(0xA2); // Fija el valor de la relacion de la tension de polarizacion del LCD a 1/9
  LCD_wr_cmd(0xA0); // El direccionamiento de la RAM de datos del display es la normal
  LCD_wr_cmd(0xC8); // El scan en las salidas COM es el normal
  LCD_wr_cmd(0x22); // Fija la relacion de resistencias interna a 2
  LCD_wr_cmd(0x2F); // Power on
  LCD_wr_cmd(0x40); // Display empieza en la linea 0
  LCD_wr_cmd(0xAF); // Display ON
  LCD_wr_cmd(0x81); // Contraste
  LCD_wr_cmd(0x17); // Valor contraste
  LCD_wr_cmd(0xA4); // Display all points normal
  LCD_wr_cmd(0xA6); // LCD Display normal
}

void LCD_update(void /*uint8_t line*/)
{
  // Cada uno de los bits del array (4096) representa el estado de uno de los 128x32 
  // pixeles de la pantalla
  int i;
  
//  if(line == 1)
//  {
    LCD_wr_cmd(0x00);     // 4 bits de la parte baja de la direccion a 0
    LCD_wr_cmd(0x10);     // 4 bits de la parte alta de la direccion a 0
    LCD_wr_cmd(0xB0);     // Pagina 0
    
    for(i = 0; i < 128; i++)
    {
      LCD_wr_data(buffer[i]);
    }
    
    LCD_wr_cmd(0x00);     // 4 bits de la parte baja de la direccion a 0
    LCD_wr_cmd(0x10);     // 4 bits de la parte alta de la direccion a 0
    LCD_wr_cmd(0xB1);     // Pagina 1
    
    for(i = 128; i  <256; i++)
    {
      LCD_wr_data(buffer[i]);
    }
//  }
  
//  else if(line == 2)
//  {
    LCD_wr_cmd(0x00);     // 4 bits de la parte baja de la direccion a 0
    LCD_wr_cmd(0x10);     // 4 bits de la parte alta de la direccion a 0
    LCD_wr_cmd(0xB2);     // Pagina 2
    
    for(i = 256; i < 384; i++)
    {
      LCD_wr_data(buffer[i]);
    }
    
    LCD_wr_cmd(0x00);     // 4 bits de la parte baja de la direccion a 0
    LCD_wr_cmd(0x10);     // 4 bits de la parte alta de la direccion a 0
    LCD_wr_cmd(0xB3);     // Pagina 3
    
    for(i = 384; i < 512; i++)
    {
      LCD_wr_data(buffer[i]);
    }
//  }
}

void LCD_borrar(uint8_t linea)
{
  int i;
	if(linea == 1){
		positionL1=0;
		for(i = 0; i < 256; i++)
		{
			buffer[i] = 0x00;
		}
	}else{
		positionL2=0;
		for(i = 256; i < 512; i++)
		{
			buffer[i] = 0x00;
		}
	}
}

void symbolToLocalBuffer_L1(uint8_t symbol)
{
  uint8_t i, value1, value2;
  uint16_t offset = 0;
  
  offset = 25*(symbol - ' ');
  
	if ((positionL1+ Arial12x12[offset])<127){
  for(i = 0; i < 12; i++)
  {
    value1 = Arial12x12[offset + i*2 + 1];
    value2 = Arial12x12[offset + i*2 + 2];
    
    buffer[i + positionL1] = value1;          // Pagina 0
    buffer[i + 128 + positionL1 ] = value2;   // Pagina 1
  }
  positionL1 += Arial12x12[offset];
  }
}

void symbolToLocalBuffer_L2(uint8_t symbol)
{
  uint8_t i, value1, value2;
  uint16_t offset = 0;
  
  offset = 25*(symbol - ' ');
  
	if ((positionL2+ Arial12x12[offset])<127){
  for(i = 0; i < 12; i++)
  {
    value1 = Arial12x12[offset + i*2 + 1];
    value2 = Arial12x12[offset + i*2 + 2];
    
    buffer[i + 256 + positionL2] = value1;   // Pagina 2
    buffer[i + 384 + positionL2] = value2;   // Pagina 3
  }
  positionL2 += Arial12x12[offset];

	}
}
void symbolToLocalBuffer(uint8_t line, uint8_t symbol)
{
  if (line == 1)
  {
    symbolToLocalBuffer_L1(symbol);
  }
  
  else if(line == 2)
  {
    symbolToLocalBuffer_L2(symbol);
  }
  
  LCD_update(); // Se actualiza el LCD
}

void escribir_linea(uint8_t line, char *buffer_escritura)
{
  int i;
  
  if (line == 1)
  {
    for(i = 0; i < strlen(buffer_escritura); i++)
    {
      symbolToLocalBuffer_L1(buffer_escritura[i]);
    }
  }
  
  else if (line == 2)
  {
    for(i = 0; i < strlen(buffer_escritura); i++)
    {
      symbolToLocalBuffer_L2(buffer_escritura[i]);
    }
  }
  LCD_update(); // Se actualiza el LCD
}
