#include "PWM.h"


osThreadId_t       tid_ThPWM;
osMessageQueueId_t pwm_Queue;

static const osThreadAttr_t attr_pwm = {
  .stack_size = 256
};


static TIM_HandleTypeDef htim1;

static void MX_TIM1_PWM_Init (void)
{

  __HAL_RCC_TIM1_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /* ---- Configura GPIO PE9 en AF1 TIM1_CH1 ---- */
  GPIO_InitTypeDef gpio = {0};
  gpio.Pin       = PWM_GPIO_PIN;
  gpio.Mode      = GPIO_MODE_AF_PP;
  gpio.Pull      = GPIO_NOPULL;
  gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
  gpio.Alternate = PWM_GPIO_AF;
  HAL_GPIO_Init(PWM_GPIO_PORT, &gpio);

  htim1.Instance = TIM1;
  htim1.Init.Prescaler         = 168 - 1;     /* 84 MHz /84 = 1 MHz */
  htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim1.Init.Period            = 1000 - 1;   /* 1 MHz /1000 = 1 kHz */
  htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  HAL_TIM_PWM_Init(&htim1);

  /* ---- Configura canal PWM ---- */
  TIM_OC_InitTypeDef sConfigOC = {0};
  sConfigOC.OCMode       = TIM_OCMODE_PWM1;
  sConfigOC.Pulse        = 0;               /* Duty 0 % de arranque */
  sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
  HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, PWM_CHANNEL);

  /* ---- Arranca PWM ---- */
  HAL_TIM_PWM_Start(&htim1, PWM_CHANNEL);
}

/* Conversión duty-cycle a valor de CCR (0-100 %) */
static inline uint32_t Duty_To_CCR(uint8_t duty)
{
  if (duty > PWM_DUTY_MAX) duty = PWM_DUTY_MAX;
  return ((uint32_t)duty * (htim1.Init.Period + 1)) / 100U;
}


static void ThPWM (void *argument)
{
  MSGQUEUE_PWM_t msg;

  while(1) {
    if (osMessageQueueGet(pwm_Queue, &msg, NULL, osWaitForever) == osOK) {
      __HAL_TIM_SET_COMPARE(&htim1, PWM_CHANNEL, Duty_To_CCR(msg.duty));
			osDelay(100);
		}
  }
}


int Init_ThPWM (void)
{
  MX_TIM1_PWM_Init();

  pwm_Queue = osMessageQueueNew(MSGQUEUE_PWM_OBJECTS, sizeof(MSGQUEUE_PWM_t), NULL);
  if (!pwm_Queue) return -1;

  tid_ThPWM = osThreadNew(ThPWM, NULL, &attr_pwm);
  return (tid_ThPWM == NULL) ? -1 : 0;
}