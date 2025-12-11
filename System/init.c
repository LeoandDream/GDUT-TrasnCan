#include "Init.h"

void init()
{
	ServoInit();
	MotorInit();
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart1, rx_buf, FRAME_MAX_LEN);
}
