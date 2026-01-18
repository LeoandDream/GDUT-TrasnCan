#include "Init.h"

void init()
{
	ServoInit();
	MotorInit();
	HAL_TIM_Base_Start_IT(&htim6);
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart1, host1_rx_buf, FRAME_MAX_LEN);
	__HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart2, host2_rx_buf, FRAME_MAX_LEN);
}

void State_Init()
{
	gripper_state = GRIPPER_STATE_STOP;
	allow_to_sort_state = Allow_To_Sort;
	sort_state = Not_Sorting;
}
