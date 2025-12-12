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

    // 3. 提取物体数量N，校验总长度（发送端每物体 5B：type(1) + centerX(2 little-endian) + centerY(2 little-endian)）
    frame->obj_num = rx_buf[2];
    uint16_t expect_len = 2 + 1 + (5 * frame->obj_num) + 1; // 帧头+N+物体数据+帧校验和
    if (rx_len != expect_len)
    {
        return 2; // 实际长度≠预期长度
    }

    // 4. 整帧校验（发送端在帧尾放置一个字节 = sum(all previous bytes) & 0xFF）
    uint8_t calc_checksum = CalculateChecksum(rx_buf, rx_len - 1);
    uint8_t frame_checksum = rx_buf[rx_len - 1];
    if (calc_checksum != frame_checksum)
    {
        return 3; // 校验和错误
    }

    // 5. 解析物体数据并填充结构体（每物体 5 字节，坐标为 little-endian）
    uint16_t idx = 3; // 跳过帧头(2)+N(1)
    for (uint8_t i = 0; i < frame->obj_num; i++)
    {
        // 类别
        frame->obj_list[i].type = rx_buf[idx++];

        // centerX little-endian: low then high
        uint8_t x_low = rx_buf[idx++];
        uint8_t x_high = rx_buf[idx++];
        uint16_t x_center = (uint16_t)x_low | ((uint16_t)x_high << 8);

        // centerY little-endian
        uint8_t y_low = rx_buf[idx++];
        uint8_t y_high = rx_buf[idx++];
        uint16_t y_center = (uint16_t)y_low | ((uint16_t)y_high << 8);

        // 存储：如果结构体只支持 x1/y1..x2/y2，可把中心放到 x1/y1，x2/y2 置 0
        frame->obj_list[i].x1 = x_center;
        frame->obj_list[i].y1 = y_center;
        frame->obj_list[i].x2 = 0;
        frame->obj_list[i].y2 = 0;
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
