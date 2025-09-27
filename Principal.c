#include "principal.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
#include "com.h"

//hilo y timer principal
static osThreadId_t         tid_ThPrincipal;
static osTimerId_t          tid_LD1Blink;          /* parpadeo LD1 2 Hz        */

//colas modulos
extern osMessageQueueId_t   lcd_Queue;             // de lcd.c               
extern osMessageQueueId_t   sens_Queue;            // de sensores.c            
extern osMessageQueueId_t   pot_Queue;             // de Pot.c                 
extern osMessageQueueId_t   pwm_Queue;             // de PWM.c                 
extern osMessageQueueId_t   mid_MsgQueue_JOY;      // de ThJoystick.c          
extern osThreadId_t         tid_Control_Led;       // de leds_N.c hilo driver LEDS RGB     
extern osMessageQueueId_t   cola_salida;     // de com.c transmisión 
extern osMessageQueueId_t   cola_entrada;  // de com.c recepción

// Variables globales del reloj
extern uint8_t Segundos, Minutos, Horas;

// Variables principal
static sistema_modo_t modo              = MODO_REPOSO;
static float          Tc                = 22.0f;   // temperatura consigna  
static float          Ta                =  4.0f;   // temperatura alarma    
static float          Ti                = 22.0f;   // temperatura interior  
static float          Te                = 22.0f;   // temperatura exterior  
static float          Ti_test           = 22.0f;   // Ti simulado en TEST   
static bool           alarma_activa     = false;
static medida_t       medidas[MEDIDAS_MAX];        // Buffer circular        
static uint8_t        idx_medida        = 0U;      // posición del buffer    
static uint32_t       t_next_lcd        = 0U;      // tick próxima actualización LCD 

//Variables del modo Programación/DEP
static uint8_t  hh_aux  = 0U;
static uint8_t  mm_aux  = 0U;
static uint8_t  ss_aux  = 0U;
static float    Tc_aux  = 22.0f;
static float    Ta_aux  =  4.0f;
static uint8_t  cursor_PD = 0U;  /* 0-hora,1-min,2-seg,3-Tc,4-Ta */
static bool     pd_first_entry = true;

//prototipos funciones
static void ThPrincipal      (void *argument);
static void actualizar_lcd   (void);
static void procesar_joystick(void);
static void procesar_sensores(void);
static void procesar_pot     (void);
static void procesar_comandos(void);
static void aplicar_control  (void);
static void gestionar_alarma (void);
static void LD1_TimerCB      (void *arg);
static void LD1_Init         (void);
static void LD1_Set          (GPIO_PinState st);


int Init_ThPrincipal (void)
{
  LD1_Init();

  tid_LD1Blink = osTimerNew(LD1_TimerCB, osTimerPeriodic, NULL, NULL);
  if (tid_LD1Blink == NULL) {
    return -1;
  }

  tid_ThPrincipal = osThreadNew(ThPrincipal, NULL, NULL);
  return (tid_ThPrincipal == NULL) ? -1 : 0;
}


static void ThPrincipal (void *argument)
{
  (void)argument;
  uint32_t ticks;

  actualizar_lcd(); 

  while (1) {
    procesar_joystick();
    procesar_comandos();
    procesar_sensores();
    procesar_pot();

    ticks = osKernelGetTickCount();
    if ((ticks - t_next_lcd) >= LCD_REFRESH_MS) {
      t_next_lcd = ticks;
      actualizar_lcd();
    }

    if ((modo == MODO_ACTIVO) || (modo == MODO_TEST)) {
      aplicar_control();
      gestionar_alarma();
    }

    osDelay(LOOP_PERIOD_MS);
  }
}

//sensores
static void procesar_sensores (void)
{
  MSGQUEUE_SENS_t sens;
  osStatus_t      status;

  status = osMessageQueueGet(sens_Queue, &sens, NULL, 0U);
  if (status == osOK) {
    Ti = sens.Ti;
    Te = sens.Te;

    if (modo == MODO_ACTIVO) {
      medidas[idx_medida].hh   = Horas;
      medidas[idx_medida].mm   = Minutos;
      medidas[idx_medida].ss   = Segundos;
      medidas[idx_medida].Ti   = Ti;
      medidas[idx_medida].Tc   = Tc;
      idx_medida               = (uint8_t)((idx_medida + 1U) % MEDIDAS_MAX);
    }
  }
}


//potenciómetros
static void procesar_pot (void)
{
  MSGQUEUE_POT_t pot;
  osStatus_t     status;

  if (modo != MODO_TEST) {
    return;
  }

  status = osMessageQueueGet(pot_Queue, &pot, NULL, 0U);
  if (status == osOK) {
    Te = pot.Te;
    Ta = pot.Ta;
  }
}

//Joystick
static void procesar_joystick (void)
{
  MSGQUEUE_JOY_t joy;
  osStatus_t     status;
  bool guardar_datos;

  status = osMessageQueueGet(mid_MsgQueue_JOY, &joy, NULL, 0U);
  if (status != osOK) {
    return;
  }

  guardar_datos = false;

  switch (modo) {
  //REPOSO 
  case MODO_REPOSO:
    if (joy.gesto == CENTER_LARGA) {
      modo = MODO_ACTIVO;
    }
    break;

  //ACTIVO 
  case MODO_ACTIVO:
    if (joy.gesto == UP_CORTA && (Tc < 30.0f)) {
      Tc += 0.5f;
    } else if (joy.gesto == DOWN_CORTA && (Tc > 2.0f)) {
      Tc -= 0.5f;
    } else if (joy.gesto == CENTER_CORTA) {
      modo = MODO_TEST;
    } else if (joy.gesto == CENTER_LARGA) {
      modo = MODO_REPOSO;
    }
    break;

  //TEST
  case MODO_TEST:
    if (joy.gesto == RIGHT_CORTA && (Ti_test < 30.0f)) {
      Ti_test += 0.5f;
    } else if (joy.gesto == LEFT_CORTA && (Ti_test > 2.0f)) {
      Ti_test -= 0.5f;
    } else if (joy.gesto == UP_CORTA && (Tc < 30.0f)) {
      Tc += 0.5f;
    } else if (joy.gesto == DOWN_CORTA && (Tc > 2.0f)) {
      Tc -= 0.5f;
    } else if (joy.gesto == CENTER_CORTA) {
      modo = MODO_PD;
    } else if (joy.gesto == CENTER_LARGA) {
      modo = MODO_REPOSO;
    }
    break;

  //PROGRAMACIÓN / DEPURACIÓN 
  case MODO_PD:
    if (pd_first_entry) {
      hh_aux = Horas; mm_aux = Minutos; ss_aux = Segundos;
      Tc_aux = Tc;    Ta_aux = Ta;
      cursor_PD = 0U;
      pd_first_entry = false;
    }

    switch (joy.gesto) {
      case RIGHT_CORTA: cursor_PD = (uint8_t)((cursor_PD + 1U) % 5U); break;
      case LEFT_CORTA:  cursor_PD = (uint8_t)((cursor_PD + 4U) % 5U); break;
      case UP_CORTA:
        if      ((cursor_PD == 0U) && (hh_aux < 23U))    hh_aux++;
        else if ((cursor_PD == 1U) && (mm_aux < 59U))    mm_aux++;
        else if ((cursor_PD == 2U) && (ss_aux < 59U))    ss_aux++;
        else if ((cursor_PD == 3U) && (Tc_aux < 30.0f))  Tc_aux += 0.5f;
        else if ((cursor_PD == 4U) && (Ta_aux < 30.0f))  Ta_aux += 0.5f;
        break;
      case DOWN_CORTA:
        if      ((cursor_PD == 0U) && (hh_aux > 0U))     hh_aux--;
        else if ((cursor_PD == 1U) && (mm_aux > 0U))     mm_aux--;
        else if ((cursor_PD == 2U) && (ss_aux > 0U))     ss_aux--;
        else if ((cursor_PD == 3U) && (Tc_aux > 2.0f))   Tc_aux -= 0.5f;
        else if ((cursor_PD == 4U) && (Ta_aux > 2.0f))   Ta_aux -= 0.5f;
        break;
      case CENTER_CORTA: guardar_datos = true;
				break;
      case CENTER_LARGA:
        modo = MODO_REPOSO;   /* salir sin guardar */
        pd_first_entry = true;
        break;
			default:
				break;
    }

    if (guardar_datos) {
      Horas = hh_aux; Minutos = mm_aux; Segundos = ss_aux;
      Tc = Tc_aux;    Ta = Ta_aux;
    }
    break;
  }
}

 //Lógica de control (PWM + LED RGB)

static void aplicar_control (void)
{
  float   x;
  uint8_t duty = 0U;

  x = Tc - ((modo == MODO_TEST) ? Ti_test : Ti);

  if (x >= 5.0f){ 
		duty = 100U;
		osThreadFlagsSet(tid_Control_Led, LedRojo_ON); }
  else if (x >= 2.0f){ 
		duty =  70U; 
		osThreadFlagsSet(tid_Control_Led, LedRojo_ON); }
  else if (x >= 0.5f){ 
		duty =  40U; 
		osThreadFlagsSet(tid_Control_Led, LedRojo_ON); }
  else if (x > -0.5f){ 
		duty =  10U; 
		osThreadFlagsSet(tid_Control_Led, LedVerde_ON);} 
  else if (x > -2.0f){ 
		duty =  40U; 
		osThreadFlagsSet(tid_Control_Led, LedAzul_ON); }
  else if (x > -5.0f){ 
		duty =  70U; 
		osThreadFlagsSet(tid_Control_Led, LedAzul_ON); }
  else { 
		duty = 100U; 
		osThreadFlagsSet(tid_Control_Led, LedAzul_ON); }

  MSGQUEUE_PWM_t pwm_msg;
	pwm_msg.duty = duty;
  osMessageQueuePut(pwm_Queue, &pwm_msg, 0U, 0U);

  if (modo == MODO_ACTIVO) {
    uint8_t pos = (uint8_t)((idx_medida + MEDIDAS_MAX - 1U) % MEDIDAS_MAX);
    medidas[pos].duty = duty;
  }
}

//alarma
static void gestionar_alarma (void)
{
  bool nueva_alarma = (Te < Ta);

  if (nueva_alarma && !alarma_activa) {
    alarma_activa = true;
    osTimerStart(tid_LD1Blink, BLINK_PERIOD_MS);
  } else if (!nueva_alarma && alarma_activa) {
    alarma_activa = false;
    osTimerStop(tid_LD1Blink);
    LD1_Set(GPIO_PIN_RESET);
  }
}

static void LD1_TimerCB (void *arg)
{
  HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
}

static uint8_t complemento(uint8_t cmd) { return (uint8_t)(~cmd); }

static void enviar_trama(uint8_t cmd, const char *payload)
{
  uint8_t msg[BUFFER_SIZE];
  uint8_t len = (uint8_t)(4U + strlen(payload));
  if (len >= BUFFER_SIZE){
		len = BUFFER_SIZE - 1U;
	} 

  
  msg[0]  = SOH;
  msg[1]  = cmd;
  msg[2]  = len;
  strncpy((char *)&msg[3], payload, len - 4U);
  msg[len - 1U] = EOT;

  osMessageQueuePut(cola_salida, &msg, 0U, 0U);
}

static void procesar_comandos(void)
{
  uint8_t frame[BUFFER_SIZE];
  osStatus_t        status;

  status = osMessageQueueGet(cola_entrada, &frame, NULL, 0U);
  if (status != osOK) {
    return;
  }


  uint8_t cmd = frame[1];
  char    payload_raw[BUFFER_SIZE];
  uint8_t payload_len = (frame[2] > 4U) ? (frame[2] - 4U) : 0U;
  memcpy(payload_raw, &frame[3], payload_len);
  payload_raw[payload_len] = '\0';
  const char *pl = payload_raw;

  switch (cmd) {
  // Puesta en hora (HH:MM:SS)
  case 0x20: {
    uint8_t hh, mm, ss;
    if (sscanf(pl, "%2hhu:%2hhu:%2hhu", &hh, &mm, &ss) == 3) {
      if ((hh < 24U) && (mm < 60U) && (ss < 60U)) {
        hh_aux = hh; mm_aux = mm; ss_aux = ss;
        enviar_trama(complemento(cmd), pl);
      }
    }
    break; }
  // Temperatura de consigna
  case 0x25: {
    float tmp;
    if (sscanf(pl, "%f", &tmp) == 1) {
      if ((tmp >= 2.0f) && (tmp <= 30.0f)) {
        Tc_aux = tmp; enviar_trama(complemento(cmd), pl);
      }
    }
    break; }
  // Temperatura de alarma
  case 0x26: {
    float tmp;
    if (sscanf(pl, "%f", &tmp) == 1) {
      if ((tmp >= 2.0f) && (tmp <= 30.0f)) {
        Ta_aux = tmp; enviar_trama(complemento(cmd), pl);
      }
    }
    break; }
  // Todas las medidas
  case 0x55: {
    char     payload[48];
    for (uint8_t i = 0U; i < MEDIDAS_MAX; i++) {
      const medida_t *m = &medidas[i];
      if ((m->hh==0U) && (m->mm==0U) && (m->ss==0U)) {
        continue; 
      }
      snprintf(payload, sizeof(payload), "%02u:%02u:%02u--Ti:%04.1f--Tc:%04.1f--D:%02u%%",
               m->hh, m->mm, m->ss, m->Ti, m->Tc, m->duty);
      enviar_trama(0xAA, payload);
    }
    break; }
  // Borrar medidas
  case 0x60:
    memset(medidas, 0, sizeof(medidas));
    enviar_trama(complemento(cmd), "");
    break;
  }
}


// LCD

static void lcd_send(uint8_t line, const char *msg)
{
  MSGQUEUE_lcd_t m;
  m.linea = line;
  strncpy(m.mensaje, msg, sizeof(m.mensaje));
  osMessageQueuePut(lcd_Queue, &m, 0U, 0U);
}

static void actualizar_lcd (void)
{
  char l1[29] = {0};
  char l2[29] = {0};

  switch (modo) {
  case MODO_REPOSO:
    snprintf(l1, sizeof(l1), "     SBM 2025      ");
    snprintf(l2, sizeof(l2), "   %02u:%02u:%02u   ", Horas, Minutos, Segundos);
    break;
  case MODO_ACTIVO: {
    uint8_t duty = medidas[(idx_medida + MEDIDAS_MAX - 1U) % MEDIDAS_MAX].duty;
    snprintf(l1, sizeof(l1), "%02u:%02u:%02u Te:%4.1f", Horas, Minutos, Segundos, Te);
    snprintf(l2, sizeof(l2), "Ti:%4.1f Tc:%4.1f D:%2u%%", Ti, Tc, duty);
    break; }
  case MODO_TEST:
    snprintf(l1, sizeof(l1), "Ti:%4.1f Te:%4.1f (MT)", Ti_test, Te);
    snprintf(l2, sizeof(l2), "Tc:%4.1f Ta:%4.1f", Tc, Ta);
    break;
  case MODO_PD:
    snprintf(l1, sizeof(l1), "       (MPD)       ");
    snprintf(l2, sizeof(l2), "%02u:%02u:%02u Tc:%4.1f Ta:%4.1f", hh_aux, mm_aux, ss_aux, Tc_aux, Ta_aux);
    break;
  }

  lcd_send(1U, l1);
  lcd_send(2U, l2);
}


// LED1
static void LD1_Init (void)
{
  GPIO_InitTypeDef gpio = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();

  gpio.Pin   = GPIO_PIN_0; /* LD1 */
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Pull  = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &gpio);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
}

static void LD1_Set (GPIO_PinState st)
{
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, st);
}
