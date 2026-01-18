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
#define DEBUG_GRIPPER 0
#define DEBUG_OS 0

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
Gripper_State gripper_state = GRIPPER_STATE_STOP;
Allow_Sort_State allow_to_sort_state = Allow_To_Sort;
Sort_State sort_state = Is_Sorting;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for Motor_Task */
osThreadId_t Motor_TaskHandle;
const osThreadAttr_t Motor_Task_attributes = {
    .name = "Motor_Task",
    .stack_size = 512 * 4,
    .priority = (osPriority_t)osPriorityLow,
};
/* Definitions for Gripper_Task */
osThreadId_t Gripper_TaskHandle;
const osThreadAttr_t Gripper_Task_attributes = {
    .name = "Gripper_Task",
    .stack_size = 512 * 4,
    .priority = (osPriority_t)osPriorityLow,
};
/* Definitions for USART_Task */
osThreadId_t USART_TaskHandle;
const osThreadAttr_t USART_Task_attributes = {
    .name = "USART_Task",
    .stack_size = 512 * 4,
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
/* Definitions for Host2_Rx_Mutex */
osMutexId_t Host2_Rx_MutexHandle;
const osMutexAttr_t Host2_Rx_Mutex_attributes = {
    .name = "Host2_Rx_Mutex"};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void Motor_Task_Function(void *argument);
void Gripper_Task_Function(void *argument);
void USART_Task_Function(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
  printf("Stack overflow in task %s\r\n", pcTaskName);
  /* Run time stack overflow checking is performed if
  configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
  called if a stack overflow is detected. */
}
/* USER CODE END 4 */

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

  /* creation of Host2_Rx_Mutex */
  Host2_Rx_MutexHandle = osMutexNew(&Host2_Rx_Mutex_attributes);

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
#if DEBUG_OS == 1
  uint16_t cnt = 0;
#endif
  /* Infinite loop */
  for (;;)
  {
#if DEBUG_OS == 1
    cnt++;
    if (cnt == 10)
    {
      cnt = 0;
      osMutexAcquire(Print_MutexHandle, osWaitForever);
      printf("Motor_Task running\r\n");
      osMutexRelease(Print_MutexHandle);
    }
#endif
    MotorX.Encoder = -Read_Encoder(3) * 360.0 * 0.2714 / (13 * 30); // 0.2714,0.4375
    MotorY.Encoder = -Read_Encoder(4) * 360.0 * 0.2595 / (13 * 30); // 0.2595,0.3987
    MotorX.Position += MotorX.Encoder;
    MotorY.Position += MotorY.Encoder;

    if (MotorX.mode == MODE_RESET)
      MotorX.Motor = -8000;
    else
      Incremental_PID(&MotorX);

    if (MotorY.mode == MODE_RESET)
      MotorY.Motor = -9000;
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
  /* USER CODE BEGIN Gripper_Task_Function */
  /* Infinite loop */
  for (;;)
  {
#if DEBUG_OS == 1
    osMutexAcquire(Print_MutexHandle, osWaitForever);
    printf("Gripper_Task running\r\n");
    osMutexRelease(Print_MutexHandle);
#endif
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

        osDelay(3000);
        gripper_state = GRIPPER_STATE_CLAMP;
        if (my_abs(MotorX.pos_error) < 5 && my_abs(MotorY.pos_error) < 5)
        {
          osDelay(4000);
          gripper_state = GRIPPER_STATE_CLAMP;
        }
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
        Servo3(0);
        Servo1(60);
        Servo2(180);
        osDelay(500);

        Servo3(Target_angle);
        osDelay(500);

        Servo1(60);
        Servo2(140);
        osDelay(500);

        Servo1_SmoothMove(60, 0, 10, 100);
        osDelay(1000);
        Servo2(180);
        osDelay(500);
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
        MotorX.Target_P = 125;
        MotorY.Target_P = 410;
        switch (Target_box)
        {
        case Hazardous:
          MotorX.Target_P = 120;
          MotorY.Target_P = 115;
          break;
        case Kitchen:
          MotorX.Target_P = 420;
          MotorY.Target_P = 115;
          break;
        case Recyclable:
          MotorX.Target_P = 420;
          MotorY.Target_P = 400;
          break;
        case Other:
          MotorX.Target_P = 150;
          MotorY.Target_P = 400;
          break;
        default:
          MotorX.Target_P = 95;
          MotorY.Target_P = 115;
        }

        // osDelay(2000);
        // gripper_state = GRIPPER_STATE_RELEASE;
        if (my_abs(MotorX.pos_error) < 5 && my_abs(MotorY.pos_error) < 5)
        {
          osDelay(2000);
          gripper_state = GRIPPER_STATE_RELEASE;
        }
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
        Servo2(175);
        osDelay(500);
        Servo3(0);
        Servo1(60);
        Servo2(175);
        osDelay(500);
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

        MotorX.Target_P = 95;
        MotorY.Target_P = 115;
        osDelay(2500);
        MotorX.mode = MODE_RESET;
        MotorY.mode = MODE_RESET;
        osDelay(1000);

        MotorX.Position = 95;
        MotorY.Position = 115;

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
#if DEBUG_OS == 1
    osMutexAcquire(Print_MutexHandle, osWaitForever);
    printf("USART_Task running\r\n");
    osMutexRelease(Print_MutexHandle);
#endif
    // Usart1Task_Run();
    // Usart2Task_Run();
    osDelay(1);
  }
  /* USER CODE END USART_Task_Function */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
