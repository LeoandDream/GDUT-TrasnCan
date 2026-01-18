#ifndef __HOST1_USART1_H
#define __HOST1_USART1_H
#include "main.h"

typedef struct
{
    uint8_t type;  // 类别（1字节）
    uint8_t angle; // 角度（1字节，0/90）
    uint16_t x;    // 坐标x（2字节）
    uint16_t y;    // 坐标y（2字节）
} ObjectData_t;

typedef struct
{
    uint8_t header[2]; // 帧头 AA BB
    ObjectData_t obj;  // 物体数据
    uint8_t checksum;  // 校验和
} FrameData_t;

typedef enum
{
    GRIPPER_STATE_CLAMP = 0x1,            // 夹取
    GRIPPER_STATE_RELEASE = 0x02,         // 释放
    GRIPPER_STATE_MOVE_TO_GRAB = 0x03,    // 抓取阶段移动
    GRIPPER_STATE_MOVE_TO_RELEASE = 0x04, // 释放阶段移动
    GRIPPER_STATE_RESET = 0x05,           // 复位
    GRIPPER_STATE_STOP = 0x00             // 停机
} Gripper_State;

typedef enum
{
    Hazardous = 0x00,
    Kitchen = 0x01,
    Recyclable = 0x02,
    Other = 0x03
} Trash_type;

#define FRAME_MAX_LEN 11

// 函数声明
uint8_t CalculateChecksum1(uint8_t *data, uint16_t len); // 计算校验和
// uint16_t PackFrame(FrameData_t *frame, uint8_t *tx_buf);                   // 打包帧为字节流
uint8_t UnpackFrame1(uint8_t *rx_buf, uint16_t rx_len, FrameData_t *frame); // 解析字节流为帧结构
void Usart1Task_Run(void);

extern uint8_t host1_rx_buf[FRAME_MAX_LEN];
extern Gripper_State gripper_state;
extern uint16_t Target_X, Target_Y, Target_angle;
extern Trash_type Target_box;
extern uint16_t host1_rx_len;
extern volatile uint8_t host1_rx_data_ready;
extern uint8_t boxes[4];

#endif
