//// @file host1_usart_task_demo.c
//// @brief USART任务示例代码
/***************这是非freeertos的接收版本******************/

// #include "main.h"

// // #define DEBUG_USART1
// int fputc(int ch, FILE *f)
// {
//     HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 1000);
//     return ch;
// }

// // 全局变量
// uint8_t rx_buf[FRAME_MAX_LEN] = {0};
// uint16_t rx_len;
// FrameData_t recv_frame = {0};
// uint8_t parse_result = 0;
// int ui = 0;

// /**
//  * @brief  计算校验和（二进制相加取低8位）
//  * @param  data: 参与校验的字节数组
//  * @param  len:  字节数组长度
//  * @retval 校验和（低8位）
//  */
// uint8_t CalculateChecksum(uint8_t *data, uint16_t len)
// {
//     uint16_t sum = 0; // 先用16位累加，避免溢出
//     for (uint16_t i = 0; i < len; i++)
//     {
//         sum += data[i];
//     }
//     return (uint8_t)(sum & 0xFF); // 取低8位
// }

// /**
//  * @brief  解析字节流为帧结构体
//  * @param  rx_buf: 接收的字节流缓冲区
//  * @param  rx_len: 字节流长度
//  * @param  frame: 输出的帧结构体
//  * @retval 0: 解析成功；1: 帧头错误；2: 长度错误；3: 校验和错误
//  */
// uint8_t UnpackFrame(uint8_t *rx_buf, uint16_t rx_len, FrameData_t *frame)
// {
//     // 新协议：帧头(2) + class_id(1) + x_middle(2) + y_middle(2) + 校验和(1) = 8 字节
//     if (rx_len != 8)
//     {
//         return 2; // 长度错误
//     }
//     if (rx_buf[0] != 0xAA || rx_buf[1] != 0xBB)
//     {
//         return 1; // 帧头错误
//     }
//     uint8_t calc_checksum = CalculateChecksum(rx_buf, rx_len - 1);
//     uint8_t frame_checksum = rx_buf[rx_len - 1];
//     if (calc_checksum != frame_checksum)
//     {
//         return 3; // 校验和错误
//     }
//     // 只解析一个目标（无obj_num字段）
//     uint8_t idx = 2;
//     frame->obj_list[0].type = rx_buf[idx++];
//     uint8_t x_low = rx_buf[idx++];
//     uint8_t x_high = rx_buf[idx++];
//     uint16_t x_center = (uint16_t)x_low | ((uint16_t)x_high << 8);
//     uint8_t y_low = rx_buf[idx++];
//     uint8_t y_high = rx_buf[idx++];
//     uint16_t y_center = (uint16_t)y_low | ((uint16_t)y_high << 8);
//     frame->obj_list[0].x1 = x_center;
//     frame->obj_list[0].y1 = y_center;
//     frame->obj_list[0].x2 = 0;
//     frame->obj_list[0].y2 = 0;
//     frame->header[0] = 0xAA;
//     frame->header[1] = 0xBB;
//     frame->checksum = 0;
//     if (ui++ == 100)
//         ui = 0;
//     return 0;
// }

// // 串口中断服务函数（stm32f1xx_it.c）
// void USART1_IRQHandler(void)
// {
//     if (isGrabbing == 0)
//     {
//         HAL_UART_IRQHandler(&huart1);
//         if (ui++ == 100)
//             ui = 0;
//         // 检测空闲中断
//         if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET)
//         {
//             __HAL_UART_CLEAR_IDLEFLAG(&huart1);
//             HAL_DMA_Abort(&hdma_usart1_rx);

//             // 读取 DMA 剩余计数并做短时稳定等待，避免主机分包导致的误触发 IDLE
//             uint32_t prev_cnt = __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
//             // 在 ISR 中只做非常短的轮询（上限迭代次数很小），以允许紧跟着到达的字节更新计数
//             // 这不是最优的非阻塞设计，但能显著降低因 USB 分包导致的帧被分割的概率
//             for (int wait = 0; wait < 200; ++wait)
//             {
//                 uint32_t cur_cnt = __HAL_DMA_GET_COUNTER(&hdma_usart1_rx);
//                 if (cur_cnt == prev_cnt)
//                     break; // 稳定了
//                 prev_cnt = cur_cnt;
//             }

//             // 计算接收长度（使用稳定后的计数）
//             rx_len = FRAME_MAX_LEN - (uint16_t)__HAL_DMA_GET_COUNTER(&hdma_usart1_rx);

// // 打印接收到的原始字节（按块格式化后一次性发送，避免逐字printf导致回显被切分）
// #ifdef DEBUG_USART1
//             {
//                 const size_t OUTBUF_SIZE = 128;
//                 char outbuf[OUTBUF_SIZE];
//                 size_t outlen = 0;
//                 // 先打印长度行
//                 int n = snprintf(outbuf + outlen, OUTBUF_SIZE - outlen, "[USART1] RX_LEN=%u\r\n", rx_len);
//                 if (n > 0)
//                 {
//                     outlen += (size_t)n;
//                     if (outlen >= OUTBUF_SIZE)
//                         outlen = OUTBUF_SIZE - 1;
//                 }
//                 // 立即发送长度行（避免后续数据过大）
//                 if (outlen > 0)
//                 {
//                     HAL_UART_Transmit(&huart1, (uint8_t *)outbuf, (uint16_t)outlen, 100);
//                 }
//                 // 逐字节格式化到缓冲区，缓冲满则发送
//                 outlen = 0;
//                 for (uint16_t i = 0; i < rx_len; i++)
//                 {
//                     // 每个字节需要至多 3 字符（"AA ")
//                     if (OUTBUF_SIZE - outlen < 4)
//                     {
//                         if (outlen > 0)
//                         {
//                             HAL_UART_Transmit(&huart1, (uint8_t *)outbuf, (uint16_t)outlen, 100);
//                             outlen = 0;
//                         }
//                     }
//                     int m = snprintf(outbuf + outlen, OUTBUF_SIZE - outlen, "%02X ", rx_buf[i]);
//                     if (m > 0)
//                     {
//                         outlen += (size_t)m;
//                         if (outlen >= OUTBUF_SIZE)
//                             outlen = OUTBUF_SIZE - 1;
//                     }
//                 }
//                 // 发送剩余内容并换行
//                 if (outlen > 0)
//                 {
//                     HAL_UART_Transmit(&huart1, (uint8_t *)outbuf, (uint16_t)outlen, 100);
//                 }
//                 const char nl[] = "\r\n";
//                 HAL_UART_Transmit(&huart1, (uint8_t *)nl, sizeof(nl) - 1, 100);
//             }
// #endif

//             // 解析帧数据
//             parse_result = UnpackFrame(rx_buf, rx_len, &recv_frame);

//             if (parse_result == 0)
//             {
//                 // 解析成功，处理recv_frame中的物体数据
//                 // 坐标线性转换并赋值到 MotorX/ MotorY
//                 float h1 = 590.0f, h2 = 480.0f;
//                 float x_raw = recv_frame.obj_list[0].x1;
//                 float y_raw = recv_frame.obj_list[0].y1;
//                 float x = (x_raw - 70.0f) / 520.0f * h1;
//                 float y = y_raw / 480.0f * h2;
//                 extern motor MotorX, MotorY;
//                 MotorX.Target_P = x;
//                 MotorY.Target_P = y;
//                 // isGrabbing = 1;
//             }
//             else
// #ifdef DEBUG_USART1
//             {
//                 // 解析失败，根据错误码处理
//                 switch (parse_result)
//                 {
//                 case 1:
//                     printf("帧头错误\r\n");
//                     break;
//                 case 2:
//                     printf("长度错误\r\n");
//                     break;
//                 case 3:
//                     printf("校验和错误\r\n");
//                     break;
//                 }
//             }
// #endif
//             /**
//             h1=590,h2=480
//             x=(x_raw-70)/520*h1
//             y=y_raw/480*h2
//             **/
//             // 重新开启DMA接收
//             HAL_UART_Receive_DMA(&huart1, rx_buf, FRAME_MAX_LEN);
//         }
//     }
// }
