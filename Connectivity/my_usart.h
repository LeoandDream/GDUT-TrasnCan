#ifndef __MY_USART_H
#define __MY_USART_H

#include "main.h"

#define DEBUG_USART1 0
#define DEBUG_USART2 1

extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

#endif
