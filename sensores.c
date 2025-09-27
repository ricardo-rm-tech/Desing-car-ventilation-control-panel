#include "sensores.h"

osThreadId_t tid_Control_sensor;
osMessageQueueId_t sens_Queue;
void ThControlsensor (void *argument);

extern ARM_DRIVER_I2C Driver_I2C1;
static ARM_DRIVER_I2C *I2Cdrv = &Driver_I2C1;

static osEventFlagsId_t busyEventFlag_lm;

static const osThreadAttr_t attr_sensor = {
  .stack_size = 256
};

static void I2C1_Init(void);
static void I2C_Callback(uint32_t event);
static void Read_LM75(uint8_t reg, uint8_t *buffer, uint8_t length, uint8_t address);

int Init_Thsensor(void)
{
  I2C1_Init();
  busyEventFlag_lm = osEventFlagsNew(NULL);

  sens_Queue = osMessageQueueNew(MSGQUEUE_SENS_OBJECTS, sizeof(MSGQUEUE_SENS_t), NULL);
  if (sens_Queue == NULL) return -1;

  tid_Control_sensor = osThreadNew(ThControlsensor, NULL, &attr_sensor);
  if (tid_Control_sensor == NULL) return -1;

  return 0;
}

void ThControlsensor(void *argument)
{
  MSGQUEUE_SENS_t datos;
  uint8_t buffer[2];
  int16_t raw;

  while (1) {
    // Leer temperatura interior
    Read_LM75(LM75_TEMP_REG, buffer, 2, LM75_INT_ADDR);
    raw = ((buffer[0] << 8) | buffer[1]) >> 7;
    datos.Ti = raw * 0.5f;

    // Leer temperatura exterior
    Read_LM75(LM75_TEMP_REG, buffer, 2, LM75_EXT_ADDR);
    raw = ((buffer[0] << 8) | buffer[1]) >> 7;
    datos.Te = raw * 0.5f;

    osMessageQueuePut(sens_Queue, &datos, 0, 0);
    osDelay(500); // 500 ms
  }
}

static void I2C1_Init(void)
{
  I2Cdrv->Initialize(I2C_Callback);
  I2Cdrv->PowerControl(ARM_POWER_FULL);
  I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD);
  I2Cdrv->Control(ARM_I2C_BUS_CLEAR, 0);
}

static void I2C_Callback(uint32_t event)
{
  if (event & ARM_I2C_EVENT_TRANSFER_DONE)
    osEventFlagsSet(busyEventFlag_lm, 0x01);
  if (event & ARM_I2C_EVENT_BUS_ERROR)
    osEventFlagsSet(busyEventFlag_lm, 0x02);
}

static void Read_LM75(uint8_t reg, uint8_t *buffer, uint8_t length, uint8_t address)
{
  I2Cdrv->MasterTransmit(address, &reg, 1, true);
  osEventFlagsWait(busyEventFlag_lm, 0x01, osFlagsWaitAny, osWaitForever);

  I2Cdrv->MasterReceive(address, buffer, length, false);
  osEventFlagsWait(busyEventFlag_lm, 0x01, osFlagsWaitAny, osWaitForever);
}