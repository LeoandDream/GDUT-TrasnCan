#ifndef __HOST2_USART_H
#define __HOST2_USART_H

#include "main.h"

typedef enum
{
    Sort_State_CMD = 0x01,         // 本程序发送信息：下层封装时判断 必须接收到分拣状态是0X00才进行封装
    Sort_Type_CMD = 0x02,          // 本程序发送信息：每次单个分拣动作过程中发送一次，发送前先发送分拣中的命令，完打开垃圾类型对应舱盖命令，下层收到时打开指定舱盖
    Allow_To_Sort_State_CMD = 0x03 // 本程序接收信息：上层分拣时判断 必须接收到封装状态是0X00才进行分拣
} CMD_Type;

typedef struct
{
    CMD_Type type; // 命令/类型
    uint8_t value;
    /*值
    type=0x01时：0x00-正在分拣，0x01-未在分拣；
    type=0x02时：0x00-有害垃圾，0x01-厨余垃圾，0x02-可回收物，0x03-其他垃圾；
    type=0x03时：0x00-允许分拣，0x01-禁止分拣；
    */
} CMD_t;

// 通用数据帧结构体：帧头+cmd+data+校验和
typedef struct
{
    uint8_t header[2]; // 帧头 AA CC
    CMD_t cmd;         // 命令/类型
    uint8_t checksum;  // 校验和
} FrameData2_t;

typedef enum
{
    Allow_To_Sort = 0x00,    // 非封装状态且遥控开启，允许分拣
    Not_Allow_To_Sort = 0x01 // 封装进行中或遥控不允许，禁止分拣
} Allow_Sort_State;

typedef enum
{
    Not_Sorting = 0x00, // 非分拣状态，允许封装
    Is_Sorting = 0x01   // 分拣进行中，禁止封装
} Sort_State;
// DMA 接收相关宏定义
#define FRAME2_MAX_LEN 11

// 外部变量声明
extern uint8_t host2_rx_buf[FRAME2_MAX_LEN];
extern uint16_t host2_rx_len;
extern FrameData2_t host2_recv_frame;
extern uint8_t host2_parse_result;
extern volatile uint8_t host2_rx_data_ready;
extern Allow_Sort_State allow_to_sort_state;
extern Sort_State sort_state;

// 对外接口函数声明
uint8_t CalculateChecksum2(uint8_t *data);
uint8_t UnpackFrame2(uint8_t *rx_buf, uint16_t _rx_len, FrameData2_t *frame);
void Usart2Task_Run(void);
void USART2_IRQHandler(void);

// 通用发送接口：cmd为命令/类型，data为数据内容，len为数据长度
void Host2Usart_SendPacket(uint8_t cmd, uint8_t data);

#endif
