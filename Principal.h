#ifndef __PRINCIPAL_H
#define __PRINCIPAL_H

#include "stm32f4xx_hal.h"
#include "cmsis_os2.h"
#include <stdint.h>
#include <stdbool.h>


#include "lcd.h"
#include "leds_N.h"
#include "sensores.h"
#include "Pot.h"
#include "ThJoystick.h"
#include "PWM.h"
#include "thClk.h"
#include "com.h"


typedef enum {
  MODO_REPOSO = 0,
  MODO_ACTIVO,
  MODO_TEST,
  MODO_PD
} sistema_modo_t;

#define MEDIDAS_MAX        10U   /* tamaño del buffer circular */

typedef struct {
  uint8_t hh;           /* hora   */
  uint8_t mm;           /* minuto */
  uint8_t ss;           /* segundo*/
  float   Ti;           /* temperatura interior */
  float   Tc;           /* temperatura consigna */
  uint8_t duty;         /* ciclo PWM (0-100 %)  */
} medida_t;

#define LOOP_PERIOD_MS      50U              /* ciclo principal         */
#define LCD_REFRESH_MS      200U             /* actualización display   */
#define BLINK_PERIOD_MS     250U             /* LD1 a 2 Hz (on/off 250) */
//#define BUFFER_SIZE   64U

int Init_ThPrincipal (void);

#endif /* __PRINCIPAL_H */
