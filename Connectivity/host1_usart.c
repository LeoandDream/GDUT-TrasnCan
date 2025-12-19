#include "main.h"

#define DEBUG_USART1 1
uint8_t isGrabbing = 0;

// 全局变量
// 互斥锁句柄（由freertos.c自动生成并初始化）
extern osMutexId_t Host1_Rx_MutexHandle;
extern osMutexId_t Print_MutexHandle;
extern osMutexId_t Gripper_StateHandle;
uint8_t host1_rx_buf[FRAME_MAX_LEN] = {0};
uint16_t host1_rx_len;
FrameData_t host1_recv_frame = {0};
uint8_t host1_parse_result = 0;
int host1_ui = 0;
volatile uint8_t host1_rx_data_ready = 0;

// 计算校验和
uint8_t CalculateChecksum(uint8_t *data, uint16_t len)
{
    uint16_t sum = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

// 解析新版单目标帧
uint8_t UnpackFrame(uint8_t *host1_rx_buf, uint16_t host1_rx_len, FrameData_t *frame)
{
    // 新协议：帧头(2) + class_id(1) + x_middle(2) + y_middle(2) + 校验和(1) = 8 字节
    if (host1_rx_len != 8)
        return 2; // 长度错误
    if (host1_rx_buf[0] != 0xAA || host1_rx_buf[1] != 0xBB)
        return 1; // 帧头错误
    uint8_t calc_checksum = CalculateChecksum(host1_rx_buf, host1_rx_len - 1);
    uint8_t frame_checksum = host1_rx_buf[host1_rx_len - 1];
    if (calc_checksum != frame_checksum)
        return 3; // 校验和错误
    uint8_t idx = 2;
    frame->obj.type = host1_rx_buf[idx++];
    uint8_t x_low = host1_rx_buf[idx++];
    uint8_t x_high = host1_rx_buf[idx++];
    uint16_t x_center = (uint16_t)x_low | ((uint16_t)x_high << 8);
    uint8_t y_low = host1_rx_buf[idx++];
    uint8_t y_high = host1_rx_buf[idx++];
    uint16_t y_center = (uint16_t)y_low | ((uint16_t)y_high << 8);
    frame->obj.x = x_center;
    frame->obj.y = y_center;
    frame->header[0] = 0xAA;
    frame->header[1] = 0xBB;
    frame->checksum = 0;
    if (host1_ui++ == 100)
        host1_ui = 0;
    return 0;
}

// USART1中断服务函数：只做接收和标记
void USART1_IRQHandler(void)
{
    osMutexAcquire(Gripper_StateHandle, osWaitForever);
    if (isGrabbing == 0)
    {
        HAL_UART_IRQHandler(&huart1);
        if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
        {
            __HAL_UART_CLEAR_IDLEFLAG(&huart1);
            HAL_DMA_Abort(&hdma_usart1_rx);
            uint32_t prev_cnt = __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
            for (int wait = 0; wait < 200; ++wait)
            {
            }
            host1_rx_len = FRAME_MAX_LEN - (uint16_t)__HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
            host1_rx_data_ready = 1; // 设置数据就绪标志
            // 不要在此处重启DMA，等任务处理完再重启
            isGrabbing = 1;
        }
    }
    osMutexRelease(Gripper_StateHandle);
}

// FreeRTOS任务：处理串口数据
void Usart1Task_Run(void)
{
    // 任务心跳打印，确认任务调度正常
    // HAL_UART_Transmit(&huart1, (uint8_t *)"USART Task Alive\r\n", 18, 100);
    // 进入临界区读取标志
    osMutexAcquire(Host1_Rx_MutexHandle, osWaitForever);
    uint8_t ready = host1_rx_data_ready;
    osMutexRelease(Host1_Rx_MutexHandle);
    if (ready)
    {
        // 拷贝数据到本地缓冲区，防止处理期间被中断修改
        osMutexAcquire(Host1_Rx_MutexHandle, osWaitForever);
        uint16_t rx_len_local = host1_rx_len;
        uint8_t rx_buf_local[FRAME_MAX_LEN];
        memcpy(rx_buf_local, host1_rx_buf, rx_len_local);
        // 先清零标志，保证即使后续代码异常也不会卡死
        host1_rx_data_ready = 0;
        host1_rx_len = 0;
        osMutexRelease(Host1_Rx_MutexHandle);

        // 打印接收到的原始字节
#if DEBUG_USART1
        {
            osMutexAcquire(Host1_Rx_MutexHandle, osWaitForever);
            const size_t OUTBUF_SIZE = 128;
            char outbuf[OUTBUF_SIZE];
            size_t outlen = 0;
            int n = snprintf(outbuf + outlen, OUTBUF_SIZE - outlen, "[USART1] RX_LEN=%u\r\n", rx_len_local);
            if (n > 0)
            {
                outlen += (size_t)n;
                if (outlen > 0)
                {
                    HAL_UART_Transmit(&huart1, (uint8_t *)outbuf, (uint16_t)outlen, 100);
                }
            }
            outlen = 0;
            for (uint16_t i = 0; i < rx_len_local; i++)
            {
                if (OUTBUF_SIZE - outlen < 4)
                {
                    HAL_UART_Transmit(&huart1, (uint8_t *)outbuf, (uint16_t)outlen, 100);
                    outlen = 0;
                }
                int m = snprintf(outbuf + outlen, OUTBUF_SIZE - outlen, "%02X ", rx_buf_local[i]);
                if (m > 0)
                    outlen += (size_t)m;
            }
            if (outlen > 0)
            {
                HAL_UART_Transmit(&huart1, (uint8_t *)outbuf, (uint16_t)outlen, 100);
            }
            const char nl[] = "\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t *)nl, sizeof(nl) - 1, 100);
            osMutexRelease(Host1_Rx_MutexHandle);
        }
#endif
        // 解析帧数据
        host1_parse_result = UnpackFrame(rx_buf_local, rx_len_local, &host1_recv_frame);
        if (host1_parse_result == 0)
        {
            // 解析成功，处理recv_frame中的物体数据
            // 坐标线性转换并赋值到 MotorX/ MotorY
            float h1 = 590.0f, h2 = 480.0f;
            float x_raw = host1_recv_frame.obj.x;
            float y_raw = host1_recv_frame.obj.y;
            float x = (x_raw - 70.0f) / 520.0f * h1;
            float y = y_raw / 480.0f * h2;
            extern motor MotorX, MotorY;
            MotorX.Target_P = x;
            MotorY.Target_P = y;
            // printf("x_raw =%d\r\n", host1_recv_frame.obj.x);
            // printf("y_raw =%d\r\n", host1_recv_frame.obj.y);
#if DEBUG_USART1
            osMutexAcquire(Print_MutexHandle, osWaitForever);
            printf("x_raw =%d\r\n", host1_recv_frame.obj.x);
            printf("y_raw =%d\r\n", host1_recv_frame.obj.y);
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
                printf("帧头错误\r\n");
                break;
            case 2:
                printf("长度错误\r\n");
                break;
            case 3:
            {
                printf("校验错误\r\n");
                break;
            }
            }
            osMutexRelease(Print_MutexHandle);
#endif
        }
        // 无论解析成功与否都重启DMA，保证后续能继续接收
        HAL_UART_Receive_DMA(&huart1, host1_rx_buf, FRAME_MAX_LEN);
    }
    osMutexAcquire(Gripper_StateHandle, osWaitForever);
    isGrabbing = 0;
    osMutexRelease(Gripper_StateHandle);
}

// DMA发送环形缓冲区及状态
// printf重定向
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 1000);
    return ch;
}
