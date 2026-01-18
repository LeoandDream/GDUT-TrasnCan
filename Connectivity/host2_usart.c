#include "host2_usart.h"

// 标准数据 AA CC 01 02 79
/**
 * @author LEO
 * @brief 上下层串口通信文件
 *
 * 通信要求：
 * 1.在遥控允许以及下层板非封装状态时，下层板向上层板发送允许分拣命令，否则发送不允许分拣命令；
 * 2.上层板在分拣进行中时，禁止封装，上层板向下层板发送正在分拣状态命令；
 * 3.上层板在每次单个分拣动作过程中向下层板发送一次分拣类型命令，指示下层板打开对应舱盖；
 * 4.上层板在分拣过程中是不会接收信息的
 * 5.两个板都会定时发送心跳包，内容为是否允许分拣状态命令/是否允许封装状态命令
 *
 *
 * 通信协议实现：
 * 帧头(2) + cmd(1) + data(1) + 校验和(1) = 5 字节
 * cmd:
 * 0x01 - 分拣状态命令---由上层板发送，指示当前分拣状态
 * 0x02 - 分拣类型命令---由上层板发送，指示当前分拣类型
 * 0x03 - 允许分拣状态命令---由下层板发送，指示当前允许分拣状态
 * data:
 * cmd=0x01时：0x00-非分拣状态，0x01-分拣状态；
 * cmd=0x02时：0x00-有害垃圾，0x01-厨余垃圾，0x02-可回收物，0x03-其他垃圾
 * cmd=0x03时：0x00-允许分拣，0x01-禁止分拣封装进行中；
 *
 *
 * 函数列表：
 * @function CalculateChecksum2 计算校验和
 * @function UnpackFrame2 解析字节流为帧结构
 * @function Usart2Task_Run 上下层串口任务运行函数
 * @function Host2Usart_SendPacket 通用发送接口
 */
#define FRAME2_LEN 5
#define FRAME2_HEADER_0 0xAA
#define FRAME2_HEADER_1 0xCC
// 互斥锁句柄
extern osMutexId_t Host2_Rx_MutexHandle;
extern osMutexId_t Print_MutexHandle;
extern osMutexId_t Gripper_StateHandle;

// DMA 接收缓冲区
uint8_t host2_rx_buf[FRAME2_MAX_LEN] = {0};
uint16_t host2_rx_len = 0;
FrameData2_t host2_recv_frame = {0};
uint8_t host2_parse_result = 1;
volatile uint8_t host2_rx_data_ready = 0;

// 校验和计算
uint8_t CalculateChecksum2(uint8_t *data)
{
    uint16_t sum = 0;
    for (uint16_t i = 0; i < 4; i++)
    {
        sum += data[i];
    }
    return (uint8_t)(sum & 0xFF);
}

// 通用数据帧解析：帧头+cmd+data+校验和
uint8_t UnpackFrame2(uint8_t *host2_rx_buf, uint16_t host2_rx_len, FrameData2_t *frame)
{
    // 帧头(2) + cmd(1) + data(1) + 校验和(1) = 5 字节
    if (host2_rx_len != FRAME2_LEN)
        return 2; // 长度错误
    if (host2_rx_buf[0] != FRAME2_HEADER_0 || host2_rx_buf[1] != FRAME2_HEADER_1)
        return 1; // 帧头错误
    uint8_t calc_checksum = CalculateChecksum2(host2_rx_buf);
    uint8_t frame_checksum = host2_rx_buf[host2_rx_len - 1];
    if (calc_checksum != frame_checksum)
        return 3; // 校验和错误
    frame->header[0] = FRAME2_HEADER_0;
    frame->header[1] = FRAME2_HEADER_1;
    frame->cmd.type = (CMD_Type)host2_rx_buf[2];
    frame->cmd.value = host2_rx_buf[3];
    frame->checksum = frame_checksum;
    return 0;
}

void Usart2Task_Run(void)
{
    osMutexAcquire(Host2_Rx_MutexHandle, osWaitForever);
    uint8_t ready = host2_rx_data_ready;
    osMutexRelease(Host2_Rx_MutexHandle);

    if (ready)
    {
        osMutexAcquire(Gripper_StateHandle, osWaitForever);
        uint8_t can_process = (allow_to_sort_state == Allow_To_Sort);
        osMutexRelease(Gripper_StateHandle);
        if (can_process)
        {
            osMutexAcquire(Host2_Rx_MutexHandle, osWaitForever);
            uint16_t rx_len_local = host2_rx_len;
            uint8_t rx_buf_local[FRAME2_MAX_LEN];
            memcpy(rx_buf_local, host2_rx_buf, rx_len_local);
            host2_rx_data_ready = 0;
            host2_rx_len = 0;
            osMutexRelease(Host2_Rx_MutexHandle);
            host2_parse_result = UnpackFrame2(rx_buf_local, rx_len_local, &host2_recv_frame);

            if (host2_parse_result == 0)
            {

#if DEBUG_USART2
                osMutexAcquire(Print_MutexHandle, osWaitForever);
                const size_t OUTBUF_SIZE = 128;
                char outbuf[OUTBUF_SIZE];
                size_t outlen = 0;
                int n = snprintf(outbuf + outlen, OUTBUF_SIZE - outlen,
                                 "[USART2] RX_LEN=%u\r\n", rx_len_local);
                if (n > 0)
                {
                    outlen += (size_t)n;
                    HAL_UART_Transmit(&huart2,
                                      (uint8_t *)outbuf,
                                      (uint16_t)outlen,
                                      100);
                }
                outlen = 0;
                for (uint16_t i = 0; i < rx_len_local; i++)
                {
                    if (OUTBUF_SIZE - outlen < 4)
                    {
                        HAL_UART_Transmit(&huart2,
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
                    HAL_UART_Transmit(&huart2,
                                      (uint8_t *)outbuf,
                                      (uint16_t)outlen,
                                      100);
                }
                const char nl[] = "\r\n";
                HAL_UART_Transmit(&huart2,
                                  (uint8_t *)nl,
                                  sizeof(nl) - 1,
                                  100);
                osMutexRelease(Print_MutexHandle);
#endif
                // 处理接收到的数据
                switch (host2_recv_frame.cmd.type)
                {
                case Sort_State_CMD:
                {
                    break;
                }
                case Sort_Type_CMD:
                {
                    break;
                }
                case Allow_To_Sort_State_CMD:
                {
                    osMutexAcquire(Gripper_StateHandle, osWaitForever);
                    if (host2_recv_frame.cmd.value == 0x00)
                    {
                        allow_to_sort_state = Allow_To_Sort;
                    }
                    else if (host2_recv_frame.cmd.value == 0x01)
                    {
                        allow_to_sort_state = Not_Allow_To_Sort;
                    }
                    osMutexRelease(Gripper_StateHandle);
                    break;
                }
                default:
                    break;
                }
            }
#if DEBUG_USART2
            else
            {
                osMutexAcquire(Print_MutexHandle, osWaitForever);
                switch (host2_parse_result)
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
            }
#endif
            HAL_UART_Receive_DMA(&huart2, host2_rx_buf, FRAME2_MAX_LEN);
        }
        else
        {
            osMutexAcquire(Host2_Rx_MutexHandle, osWaitForever);
            host2_rx_data_ready = 0;
            host2_rx_len = 0;
            osMutexRelease(Host2_Rx_MutexHandle);
            HAL_UART_Receive_DMA(&huart2, host2_rx_buf, FRAME2_MAX_LEN);
        }
    }
}

// 通用发送数据包接口实现
void Host2Usart_SendPacket(uint8_t cmd, uint8_t data)
{
    // 固定帧长：帧头(2) + cmd(1) + data(1) + 校验(1) = 5 字节
    uint8_t tx_buf[5] = {0};
    uint8_t data_state = 0;
    tx_buf[0] = 0xAA;
    tx_buf[1] = 0xCC;
    tx_buf[2] = cmd;
    if (cmd == Sort_State_CMD)
    {
        if (data == 0x00 || data == 0x01)
        {
            data_state = 1;
        }
    }
    if (cmd == Sort_Type_CMD)
    {
        if (data == 0x00 || data == 0x01 || data == 0x02 || data == 0x03)
        {
            data_state = 1;
        }
    }
    if (cmd == Allow_To_Sort_State_CMD)
    {
        if (data == 0x00 || data == 0x01)
        {
            data_state = 1;
        }
    }
    if (data_state == 1)
    {
        tx_buf[3] = data;
        // 校验和
        tx_buf[4] = CalculateChecksum2(tx_buf);
        // 发送
        HAL_UART_Transmit(&huart2, tx_buf, 5, 100);
    }
}
