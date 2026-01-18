#include "host1_usart.h"

// 标准数据 AA BB 00 00 64 00 32 00 FB
#define FRAME1_LEN 9
#define FRAME1_HEADER_0 0xAA
#define FRAME1_HEADER_1 0xBB
// 互斥锁句柄
extern osMutexId_t Host1_Rx_MutexHandle;
extern osMutexId_t Print_MutexHandle;
extern osMutexId_t Gripper_StateHandle;

uint8_t host1_rx_buf[FRAME_MAX_LEN] = {0};
uint16_t host1_rx_len;
FrameData_t host1_recv_frame = {0};
uint8_t host1_parse_result = 0;
volatile uint8_t host1_rx_data_ready = 0;
uint16_t Target_X, Target_Y, Target_angle;
Trash_type Target_box;
uint8_t boxes[4] = {Hazardous, Kitchen, Recyclable, Other};

uint8_t CalculateChecksum1(uint8_t *data, uint16_t len)
{
    uint16_t sum = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

uint8_t UnpackFrame1(uint8_t *host1_rx_buf, uint16_t rx_len, FrameData_t *frame)
{
    // 新协议：帧头(2) + class_id(1) + angle(1) + x_middle(2) + y_middle(2) + 校验和(1) = 9 字节
    if (rx_len != FRAME1_LEN)
        return 2; // 长度错误
    if (host1_rx_buf[0] != FRAME1_HEADER_0 || host1_rx_buf[1] != FRAME1_HEADER_1)
        return 1; // 帧头错误
    uint8_t calc_checksum = CalculateChecksum1(host1_rx_buf, rx_len - 1);
    uint8_t frame_checksum = host1_rx_buf[rx_len - 1];
    if (calc_checksum != frame_checksum)
        return 3; // 校验和错误
    uint8_t idx = 2;
    frame->obj.type = host1_rx_buf[idx++];
    frame->obj.angle = host1_rx_buf[idx++];
    uint8_t x_low = host1_rx_buf[idx++];
    uint8_t x_high = host1_rx_buf[idx++];
    uint16_t x_center = (uint16_t)x_low | ((uint16_t)x_high << 8);
    uint8_t y_low = host1_rx_buf[idx++];
    uint8_t y_high = host1_rx_buf[idx++];
    uint16_t y_center = (uint16_t)y_low | ((uint16_t)y_high << 8);
    frame->obj.x = x_center;
    frame->obj.y = y_center;
    frame->header[0] = FRAME1_HEADER_0;
    frame->header[1] = FRAME1_HEADER_1;
    frame->checksum = 0;
    return 0;
}

void Usart1Task_Run(void)
{
    // 进入临界区读取标志
    osMutexAcquire(Host1_Rx_MutexHandle, osWaitForever);
    uint8_t ready = host1_rx_data_ready;
    osMutexRelease(Host1_Rx_MutexHandle);
    if (ready)
    {
        osMutexAcquire(Gripper_StateHandle, osWaitForever);
        uint8_t can_process = (gripper_state == GRIPPER_STATE_STOP);
        osMutexRelease(Gripper_StateHandle);

        if (can_process)
        {
            // 拷贝数据到本地缓冲区
            osMutexAcquire(Host1_Rx_MutexHandle, osWaitForever);
            uint16_t rx_len_local = host1_rx_len;
            uint8_t rx_buf_local[FRAME_MAX_LEN];
            memcpy(rx_buf_local, host1_rx_buf, rx_len_local);
            host1_rx_data_ready = 0;
            host1_rx_len = 0;
            osMutexRelease(Host1_Rx_MutexHandle);

#if DEBUG_USART1
            {
                osMutexAcquire(Host1_Rx_MutexHandle, osWaitForever);
                const size_t OUTBUF_SIZE = 128;
                char outbuf[OUTBUF_SIZE];
                size_t outlen = 0;
                int n = snprintf(outbuf + outlen, OUTBUF_SIZE - outlen,
                                 "[USART1] RX_LEN=%u\r\n", rx_len_local);
                if (n > 0)
                {
                    outlen += (size_t)n;
                    HAL_UART_Transmit(&huart1,
                                      (uint8_t *)outbuf,
                                      (uint16_t)outlen,
                                      100);
                }
                outlen = 0;
                for (uint16_t i = 0; i < rx_len_local; i++)
                {
                    if (OUTBUF_SIZE - outlen < 4)
                    {
                        HAL_UART_Transmit(&huart1,
                                          (uint8_t *)outbuf,
                                          (uint16_t)outlen,
                                          100);
                        outlen = 0;
                    }
                    int m = snprintf(outbuf + outlen,
                                     OUTBUF_SIZE - outlen,
                                     "%02X ", rx_buf_local[i]);
                    if (m > 0)
                        outlen += (size_t)m;
                }
                if (outlen > 0)
                {
                    HAL_UART_Transmit(&huart1,
                                      (uint8_t *)outbuf,
                                      (uint16_t)outlen,
                                      100);
                }
                const char nl[] = "\r\n";
                HAL_UART_Transmit(&huart1,
                                  (uint8_t *)nl,
                                  sizeof(nl) - 1,
                                  100);
                osMutexRelease(Host1_Rx_MutexHandle);
            }
#endif
            host1_parse_result =
                UnpackFrame1(rx_buf_local, rx_len_local, &host1_recv_frame);

            if (host1_parse_result == 0)
            {
                Target_X = host1_recv_frame.obj.x;
                Target_Y = host1_recv_frame.obj.y;
                Target_angle = host1_recv_frame.obj.angle;
                switch (host1_recv_frame.obj.type)
                {
                case 0:
                case 1:
                    Target_box = Hazardous;
                    break;
                case 2:
                case 3:
                case 7:
                    Target_box = Kitchen;
                    break;
                case 4:
                case 8:
                    Target_box = Recyclable;
                    break;
                case 5:
                case 6:
                    Target_box = Other;
                    break;
                }
                osMutexAcquire(Gripper_StateHandle, osWaitForever);
                gripper_state = GRIPPER_STATE_MOVE_TO_GRAB;
                osMutexRelease(Gripper_StateHandle);

#if DEBUG_USART1
                osMutexAcquire(Print_MutexHandle, osWaitForever);
                printf("x_raw =%d\r\n", host1_recv_frame.obj.x);
                printf("y_raw =%d\r\n", host1_recv_frame.obj.y);
                printf("type  =%d\r\n", host1_recv_frame.obj.type);
                printf("angle=%d\r\n", host1_recv_frame.obj.angle);
                osMutexRelease(Print_MutexHandle);
#endif
            }
            else
            {
#if DEBUG_USART1
                osMutexAcquire(Print_MutexHandle, osWaitForever);
                switch (host1_parse_result)
                {
                case 1:
                {
                    printf("帧头错误\r\n");
                    break;
                }
                case 2:
                {
                    printf("长度错误\r\n");
                    break;
                }
                case 3:
                {
                    printf("校验错误\r\n");
                    break;
                }
                }
                osMutexRelease(Print_MutexHandle);
#endif
            }
        }
        // 处理完后重启 DMA
        HAL_UART_Receive_DMA(&huart1, host1_rx_buf, FRAME_MAX_LEN);
    }
    else
    {
        // can_process!=1，丢弃旧包，直接重启DMA，不处理
        osMutexAcquire(Host1_Rx_MutexHandle, osWaitForever);
        host1_rx_data_ready = 0;
        host1_rx_len = 0;
        osMutexRelease(Host1_Rx_MutexHandle);
        HAL_UART_Receive_DMA(&huart1, host1_rx_buf, FRAME_MAX_LEN);
    }
}
