#ifndef __HOST1_USART1_H
#define __HOST1_USART1_H
#include "main.h"

typedef struct
{
    uint8_t type; // 类别（1字节）
    uint16_t x;   // 坐标x（2字节）
    uint16_t y;   // 坐标y（2字节）
} ObjectData_t;

typedef struct
{
    uint8_t header[2]; // 帧头 AA BB
    ObjectData_t obj;  // 物体列表（最大255个）
    uint8_t checksum;  // 校验和
} FrameData_t;

// 帧最大长度（每物体5字节+帧尾校验：2+1+5*255+1=1278）
#define FRAME_MAX_LEN 1278

// 函数声明
uint8_t CalculateChecksum(uint8_t *data, uint16_t len); // 计算校验和
// uint16_t PackFrame(FrameData_t *frame, uint8_t *tx_buf);                   // 打包帧为字节流
uint8_t UnpackFrame(uint8_t *rx_buf, uint16_t rx_len, FrameData_t *frame); // 解析字节流为帧结构
void Usart1Task_Run(void);

extern uint8_t host1_rx_buf[FRAME_MAX_LEN];
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

#endif
