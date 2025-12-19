#include "Init.h"

void init()
{
	ServoInit();
	MotorInit();
	HAL_TIM_Base_Start_IT(&htim6);
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart1, host1_rx_buf, FRAME_MAX_LEN);
}
