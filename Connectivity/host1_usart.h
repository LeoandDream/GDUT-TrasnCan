#ifndef __HOST1_USART1_H
#define __HOST1_USART1_H
#include "main.h"

typedef struct
{
    uint8_t type; // 类别（1字节）
    uint16_t x1;  // 坐标x1（2字节）
    uint16_t y1;  // 坐标y1（2字节）
    uint16_t x2;  // 坐标x2（2字节）
    uint16_t y2;  // 坐标y2（2字节）
} ObjectData_t;

typedef struct
{
    uint8_t header[2];          // 帧头 AA BB
    uint8_t obj_num;            // 物体数量N
    ObjectData_t obj_list[255]; // 物体列表（最大255个）
    uint8_t checksum;           // 校验和
} FrameData_t;

// 帧最大长度（采用每物体带校验位且无帧尾校验：2 + 1 + 10*255 = 2553 字节）
// 每个物体占用字节数：1(type) + 2 + 2 + 2 + 2 = 9 字节，外加 1 字节的每物体校验
#define FRAME_MAX_LEN 2553

// 函数声明
uint8_t CalculateChecksum(uint8_t *data, uint16_t len); // 计算校验和
// uint16_t PackFrame(FrameData_t *frame, uint8_t *tx_buf);                   // 打包帧为字节流
uint8_t UnpackFrame(uint8_t *rx_buf, uint16_t rx_len, FrameData_t *frame); // 解析字节流为帧结构

extern uint8_t rx_buf[FRAME_MAX_LEN];
extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;

#endif
