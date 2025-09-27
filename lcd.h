#ifndef __LCD_H
#define __LCD_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include "Driver_SPI.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int Init_ThLCD (void);                                                  // Funcion de creacion e inicializacion del thread asociado al LCD
void LCD_Init(void);                                                    // Funcion que se encarga de realizar el reset e inicializar el LCD
void LCD_reset(void);                                                   // Funcion para inicializar el LCD
void delay(uint32_t n_microsegundos);                                   // Funcion para realizar esperas en microsegundos
void LCD_wr_data(unsigned char data);                                   // Funcion para escribir un dato en el LCD
void LCD_wr_cmd(unsigned char cmd);                                     // Funcion para escribir un comando en el LCD
void LCD_setup(void);                                                   // Funcion que envia al LCD una secuencia de operaciones especifica
void LCD_update(void /*uint8_t line*/);                                 // Funcion que permite copiar la informacion de un array de datos global (buffer) tipo unsigned char de 512 elementos a la memoria de la linea 1 del display LCD
void LCD_borrar(uint8_t linea);                                                  // Funcion para borrar/limpar la pantalla del LCD
void symbolToLocalBuffer_L1(uint8_t symbol);                            // Funcion que permite escribir un simbolo/caracter en la linea superior (1) del LCD
void symbolToLocalBuffer_L2(uint8_t symbol);                            // Funcion que permite escribir un simbolo/caracter en la linea inferior (2) del LCD
void symbolToLocalBuffer(uint8_t line, uint8_t symbol);                 // Funcion que permite escribir simbolos/texto en ambas lineas del LCD
void escribir_linea(uint8_t line, char *buffer_escritura);              // Funcion que escribe en las lineas 1 y 2 del LCD el valor de 2 variables junto con texto

typedef struct                      // object data type
{
  char mensaje[29]; 
	uint8_t linea;
} MSGQUEUE_lcd_t;

#endif
