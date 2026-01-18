#include "my_usart.h"

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        // 先读取DMA剩余计数（此时DMA仍在）
        uint16_t dma_remain =
            (uint16_t)__HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
        // 再停止DMA
        HAL_DMA_Abort(&hdma_usart1_rx);
        // 计算实际接收长度
        host1_rx_len = FRAME_MAX_LEN - dma_remain;
        // 通知任务
        host1_rx_data_ready = 1;
    }
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart2);
        // 先读取DMA剩余计数（此时DMA仍在）
        uint16_t dma_remain = 
						(uint16_t)__HAL_DMA_GET_COUNTER(&hdma_usart2_rx);
        // 再停止DMA
        HAL_DMA_Abort(&hdma_usart2_rx);
        // 计算实际接收长度
        host2_rx_len = FRAME2_MAX_LEN - dma_remain;
        // 通知任务
        host2_rx_data_ready = 1;
    }
}

int fputc(int ch, FILE *f)
{
#if DEBUG_USART1 == 1
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 1000);
#endif

#if DEBUG_USART2 == 1
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 1000);
#endif
    return ch;
}
