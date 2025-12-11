#include "main.h"

int fputc(int ch, FILE *f)
{

    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 1000);
    return ch;
}

// 全局变量
uint8_t rx_buf[FRAME_MAX_LEN] = {0};
uint16_t rx_len;
FrameData_t recv_frame = {0};
uint8_t parse_result = 0;
int ui = 0;

/**
 * @brief  计算校验和（二进制相加取低8位）
 * @param  data: 参与校验的字节数组
 * @param  len:  字节数组长度
 * @retval 校验和（低8位）
 */
uint8_t CalculateChecksum(uint8_t *data, uint16_t len)
{
    uint16_t sum = 0; // 先用16位累加，避免溢出
    for (uint16_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF); // 取低8位
}

/**
 * @brief  解析字节流为帧结构体
 * @param  rx_buf: 接收的字节流缓冲区
 * @param  rx_len: 字节流长度
 * @param  frame: 输出的帧结构体
 * @retval 0: 解析成功；1: 帧头错误；2: 长度错误；3: 校验和错误
 */
uint8_t UnpackFrame(uint8_t *rx_buf, uint16_t rx_len, FrameData_t *frame)
{
    // 1. 校验最小长度（帧头2 + N(1) = 3 字节；无物体时最小长度）
    if (rx_len < 3)
    {
        return 2; // 长度不足
    }

    // 2. 验证帧头
    if (rx_buf[0] != 0xAA || rx_buf[1] != 0xBB)
    {
        return 1; // 帧头错误
    }

    // 3. 提取物体数量N，校验总长度
    frame->obj_num = rx_buf[2];
    // 每个物体占用 9 字节数据 + 1 字节物体校验 = 10 字节
    uint16_t expect_len = 2 + 1 + (10 * frame->obj_num) + 1; // 帧头+N+物体数据(含每物体校验)+帧校验和
    if (rx_len != expect_len)
    {
        return 2; // 实际长度≠预期长度
    }

    // 4. 首先按每物体校验位逐个验证（每物体的校验仅覆盖该物体的 9 字节数据）
    uint16_t idx_check = 3; // 跳过帧头(2)+N(1)
    for (uint8_t i = 0; i < frame->obj_num; i++)
    {
        // 每个物体的 9 字节数据起始下标
        uint16_t obj_start = idx_check;
        // 9 字节：type + x1(2) + y1(2) + x2(2) + y2(2)
        // 物体校验位在第 10 字节位置
        uint8_t calc_obj_checksum = CalculateChecksum(&rx_buf[obj_start], 9);
        uint8_t obj_checksum = rx_buf[obj_start + 9];
        if (calc_obj_checksum != obj_checksum)
        {
            return 3; // 物体校验错误（复用 3 = 校验和错误）
        }
        idx_check += 10; // 跳过当前物体的 10 字节（含校验）
    }

    // 5. 解析物体数据并填充结构体（跳过每物体的校验字节）；不再进行整帧尾部校验
    uint16_t idx = 3; // 跳过帧头(2)+N(1)
    for (uint8_t i = 0; i < frame->obj_num; i++)
    {
        // 类别（单个idx++，无冲突）
        frame->obj_list[i].type = rx_buf[idx++];

        // x1（大端解析：拆分高/低字节的idx操作）
        uint8_t x1_high = rx_buf[idx++]; // 先取高字节
        uint8_t x1_low = rx_buf[idx++];  // 再取低字节
        frame->obj_list[i].x1 = (x1_high << 8) | x1_low;

        // y1（大端解析：同理拆分）
        uint8_t y1_high = rx_buf[idx++];
        uint8_t y1_low = rx_buf[idx++];
        frame->obj_list[i].y1 = (y1_high << 8) | y1_low;

        // x2（大端解析：同理拆分）
        uint8_t x2_high = rx_buf[idx++];
        uint8_t x2_low = rx_buf[idx++];
        frame->obj_list[i].x2 = (x2_high << 8) | x2_low;

        // y2（大端解析：同理拆分）
        uint8_t y2_high = rx_buf[idx++];
        uint8_t y2_low = rx_buf[idx++];
        frame->obj_list[i].y2 = (y2_high << 8) | y2_low;

        // 跳过该物体的校验字节（已在前面校验过）
        idx++;
    }

    // 6. 填充帧头（无尾部帧校验）
    frame->header[0] = 0xAA;
    frame->header[1] = 0xBB;
    frame->checksum = 0; // 无整帧校验
    if (ui++ == 100)
        ui = 0;
    return 0; // 解析成功
}

// 串口中断服务函数（stm32f1xx_it.c）
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
    if (ui++ == 100)
        ui = 0;
    // 检测空闲中断
    if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
    {
        __HAL_UART_CLEAR_IDLEFLAG(&huart1);
        HAL_DMA_Abort(&hdma_usart1_rx);

        // 读取 DMA 剩余计数并做短时稳定等待，避免主机分包导致的误触发 IDLE
        uint32_t prev_cnt = __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
        // 在 ISR 中只做非常短的轮询（上限迭代次数很小），以允许紧跟着到达的字节更新计数
        // 这不是最优的非阻塞设计，但能显著降低因 USB 分包导致的帧被分割的概率
        for (int wait = 0; wait < 200; ++wait)
        {
            uint32_t cur_cnt = __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
            if (cur_cnt == prev_cnt)
                break; // 稳定了
            prev_cnt = cur_cnt;
        }

        // 计算接收长度（使用稳定后的计数）
        rx_len = FRAME_MAX_LEN - (uint16_t)__HAL_DMA_GET_COUNTER(&hdma_usart1_rx);

        // 打印接收到的原始字节（按块格式化后一次性发送，避免逐字printf导致回显被切分）
        {
            const size_t OUTBUF_SIZE = 128;
            char outbuf[OUTBUF_SIZE];
            size_t outlen = 0;

            // 先打印长度行
            int n = snprintf(outbuf + outlen, OUTBUF_SIZE - outlen, "[USART1] RX_LEN=%u\r\n", rx_len);
            if (n > 0)
            {
                outlen += (size_t)n;
                if (outlen >= OUTBUF_SIZE)
                    outlen = OUTBUF_SIZE - 1;
            }
            // 立即发送长度行（避免后续数据过大）
            if (outlen > 0)
            {
                HAL_UART_Transmit(&huart1, (uint8_t *)outbuf, (uint16_t)outlen, 100);
            }

            // 逐字节格式化到缓冲区，缓冲满则发送
            outlen = 0;
            for (uint16_t i = 0; i < rx_len; i++)
            {
                // 每个字节需要至多 3 字符（"AA ")
                if (OUTBUF_SIZE - outlen < 4)
                {
                    if (outlen > 0)
                    {
                        HAL_UART_Transmit(&huart1, (uint8_t *)outbuf, (uint16_t)outlen, 100);
                        outlen = 0;
                    }
                }
                int m = snprintf(outbuf + outlen, OUTBUF_SIZE - outlen, "%02X ", rx_buf[i]);
                if (m > 0)
                {
                    outlen += (size_t)m;
                    if (outlen >= OUTBUF_SIZE)
                        outlen = OUTBUF_SIZE - 1;
                }
            }
            // 发送剩余内容并换行
            if (outlen > 0)
            {
                HAL_UART_Transmit(&huart1, (uint8_t *)outbuf, (uint16_t)outlen, 100);
            }
            const char nl[] = "\r\n";
            HAL_UART_Transmit(&huart1, (uint8_t *)nl, sizeof(nl) - 1, 100);
        }

        // 解析帧数据
        parse_result = UnpackFrame(rx_buf, rx_len, &recv_frame);
        if (parse_result == 0)
        {
            // 解析成功，处理recv_frame中的物体数据
            // 示例：打印物体数量（实际项目中替换为业务逻辑）
            printf("接收物体数量：%d\r\n", recv_frame.obj_num);
        }
        else
        {
            // 解析失败，根据错误码处理
            switch (parse_result)
            {
            case 1:
                printf("帧头错误\r\n");
                break;
            case 2:
                printf("长度错误\r\n");
                break;
            case 3:
                printf("校验和错误\r\n");
                break;
            }
        }

        // 重新开启DMA接收
        HAL_UART_Receive_DMA(&huart1, rx_buf, FRAME_MAX_LEN);
    }
}
