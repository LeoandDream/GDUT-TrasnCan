/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
Gripper_State gripper_state = GRIPPER_STATE_STOP;
#define DEBUG_GRIPPER 0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for Motor_Task */
osThreadId_t Motor_TaskHandle;
const osThreadAttr_t Motor_Task_attributes = {
    .name = "Motor_Task",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityLow,
};
/* Definitions for Gripper_Task */
osThreadId_t Gripper_TaskHandle;
const osThreadAttr_t Gripper_Task_attributes = {
    .name = "Gripper_Task",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityLow,
};
/* Definitions for USART_Task */
osThreadId_t USART_TaskHandle;
const osThreadAttr_t USART_Task_attributes = {
    .name = "USART_Task",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityLow,
};
/* Definitions for Gripper_State */
osMutexId_t Gripper_StateHandle;
const osMutexAttr_t Gripper_State_attributes = {
    .name = "Gripper_State"};
/* Definitions for Host1_Rx_Mutex */
osMutexId_t Host1_Rx_MutexHandle;
const osMutexAttr_t Host1_Rx_Mutex_attributes = {
    .name = "Host1_Rx_Mutex"};
/* Definitions for Print_Mutex */
osMutexId_t Print_MutexHandle;
const osMutexAttr_t Print_Mutex_attributes = {
    .name = "Print_Mutex"};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void Motor_Task_Function(void *argument);
void Gripper_Task_Function(void *argument);
void USART_Task_Function(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of Gripper_State */
  Gripper_StateHandle = osMutexNew(&Gripper_State_attributes);

  /* creation of Host1_Rx_Mutex */
  Host1_Rx_MutexHandle = osMutexNew(&Host1_Rx_Mutex_attributes);

  /* creation of Print_Mutex */
  Print_MutexHandle = osMutexNew(&Print_Mutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of Motor_Task */
  Motor_TaskHandle = osThreadNew(Motor_Task_Function, NULL, &Motor_Task_attributes);

  /* creation of Gripper_Task */
  Gripper_TaskHandle = osThreadNew(Gripper_Task_Function, NULL, &Gripper_Task_attributes);

  /* creation of USART_Task */
  USART_TaskHandle = osThreadNew(USART_Task_Function, NULL, &USART_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_Motor_Task_Function */
/**
 * @brief Function implementing the Motor_Task thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Motor_Task_Function */
void Motor_Task_Function(void *argument)
{
  /* USER CODE BEGIN Motor_Task_Function */
  /* Infinite loop */
  for (;;)
  {
    MotorX.Encoder = Read_Encoder(3);
    MotorY.Encoder = Read_Encoder(4);
    MotorX.Position += MotorX.Encoder;
    MotorY.Position += MotorY.Encoder;

    if (MotorX.mode == MODE_RESET)
      MotorX.Motor = 5000;
    else
      Incremental_PID(&MotorX);

    if (MotorY.mode == MODE_RESET)
      MotorY.Motor = 5000;
    else
      Incremental_PID(&MotorY);

    if (MotorX.mode == MODE_STOP)
      MotorX.Motor = 0;

    if (MotorY.mode == MODE_STOP)
      MotorY.Motor = 0;

    Set_Pwm(&MotorX, &MotorY);
    osDelay(1);
  }
  /* USER CODE END Motor_Task_Function */
}

/* USER CODE BEGIN Header_Gripper_Task_Function */
/**
 * @brief Function implementing the Gripper_Task thread.
 * just test now
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Gripper_Task_Function */
void Gripper_Task_Function(void *argument)
{
  for (;;)
  {
    if (osMutexAcquire(Gripper_StateHandle, 0) == osOK)
    {

      switch (gripper_state)
      {
      case GRIPPER_STATE_MOVE_TO_GRAB:
      {
#if DEBUG_GRIPPER
        osMutexAcquire(Print_MutexHandle, osWaitForever);
        printf("GRIPPER_STATE_MOVE_TO_GRAB running\r\n");
        osMutexRelease(Print_MutexHandle);
#endif
        MotorX.mode = MODE_POSITION_SINGLE;
        MotorY.mode = MODE_POSITION_SINGLE;
        MotorX.Target_P = Target_X;
        MotorY.Target_P = Target_Y;
        osDelay(2000);
        gripper_state = GRIPPER_STATE_CLAMP;
        break;
      }
      case GRIPPER_STATE_CLAMP:
      {
#if DEBUG_GRIPPER
        osMutexAcquire(Print_MutexHandle, osWaitForever);
        printf("GRIPPER_STATE_CLAMP running\r\n");
        osMutexRelease(Print_MutexHandle);
#endif
        MotorX.mode = MODE_STOP;
        MotorY.mode = MODE_STOP;
        Servo1(60);
        Servo2(180);
        osDelay(1000);

        Servo1(60);
        Servo2(150);
        osDelay(1000);
        gripper_state = GRIPPER_STATE_MOVE_TO_RELEASE;
        break;
      }

      case GRIPPER_STATE_MOVE_TO_RELEASE:
      {
#if DEBUG_GRIPPER
        osMutexAcquire(Print_MutexHandle, osWaitForever);
        printf("GRIPPER_STATE_MOVE_TO_RELEASE running\r\n");
        osMutexRelease(Print_MutexHandle);
#endif
        MotorX.mode = MODE_POSITION_SINGLE;
        MotorY.mode = MODE_POSITION_SINGLE;
        MotorX.Target_P = 71;
        MotorY.Target_P = 0;
        gripper_state = GRIPPER_STATE_RESET;
        osDelay(2000);
        break;
      }

      case GRIPPER_STATE_RELEASE:
      {
#if DEBUG_GRIPPER
        osMutexAcquire(Print_MutexHandle, osWaitForever);
        printf("GRIPPER_STATE_RELEASE running\r\n");
        osMutexRelease(Print_MutexHandle);
#endif
        Servo1(0);
        osDelay(1000);
        Servo2(180);
        osDelay(1000);
        gripper_state = GRIPPER_STATE_RESET;
        break;
      }
      case GRIPPER_STATE_RESET:
      {
#if DEBUG_GRIPPER
        osMutexAcquire(Print_MutexHandle, osWaitForever);
        printf("GRIPPER_STATE_RESET running\r\n");
        osMutexRelease(Print_MutexHandle);
#endif
        MotorX.mode = MODE_RESET;
        MotorY.mode = MODE_RESET;
        osDelay(2000);
        MotorX.mode = MODE_STOP;
        MotorY.mode = MODE_STOP;
        gripper_state = GRIPPER_STATE_STOP;
        break;
      }
      case GRIPPER_STATE_STOP:
      {
#if DEBUG_GRIPPER
        osMutexAcquire(Print_MutexHandle, osWaitForever);
        printf("GRIPPER_STATE_STOP\r\n");
        osMutexRelease(Print_MutexHandle);
#endif
        (void)0;
        osDelay(500);
      }
      default:
        break;
      }
      osMutexRelease(Gripper_StateHandle);
    }
    osDelay(1);
  }
  /* USER CODE END Gripper_Task_Function */
}

/* USER CODE BEGIN Header_USART_Task_Function */
/**
 * @brief Function implementing the USART_Task thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_USART_Task_Function */
void USART_Task_Function(void *argument)
{
  /* USER CODE BEGIN USART_Task_Function */
  /* Infinite loop */
  for (;;)
  {
    Usart1Task_Run();
    osDelay(1);
  }
  /* USER CODE END USART_Task_Function */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
