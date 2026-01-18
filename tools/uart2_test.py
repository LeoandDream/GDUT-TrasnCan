#!/usr/bin/env python3
"""
串口2协议测试脚本（适配host2_usart.c协议）
用法：
  python uart2_test.py --port COM6 --baud 115200 --count 100 --interval 0.05 --error_rate 0.2
"""
import argparse
import struct
import serial
import sys
import time
import random

def build_frame(cmd=0x01, data=0x00):
    """
    构造协议帧: 帧头(0xAA 0xCC) + cmd(1) + data(1) + 校验和(1)
    """
    frame = bytearray()
    frame.extend([0xAA, 0xCC])
    frame.append(cmd & 0xFF)
    frame.append(data & 0xFF)
    checksum = sum(frame) & 0xFF
    frame.append(checksum)
    return frame

def build_error_frame(frame, error_type='checksum'):
    f = bytearray(frame)
    if error_type == 'checksum':
        # 故意破坏校验和
        f[-1] = (f[-1] + random.randint(1, 255)) & 0xFF
    elif error_type == 'header':
        # 故意破坏帧头
        f[0] = 0x00
    elif error_type == 'length':
        # 缩短帧长度
        if len(f) > 3:
            f = f[:-1]
    return f

def main():
    parser = argparse.ArgumentParser(description='UART2协议压力测试脚本（host2_usart.c协议）')
    parser.add_argument('--port', required=True, help='Serial port, e.g. COM6')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--count', type=int, default=10, help='How many frames to send')
    parser.add_argument('--interval', type=float, default=0.05, help='Interval between frames (s)')
    parser.add_argument('--timeout', type=float, default=0.5, help='Read echo timeout (s)')
    parser.add_argument('--random', action='store_true', help='Randomize cmd/data for each frame')
    parser.add_argument('--error_rate', type=float, default=0.2, help='Ratio of error frames (0~1)')
    parser.add_argument('--error_types', type=str, default='checksum,header,length', help='Comma separated error types: checksum,header,length')
    args = parser.parse_args()
    error_types = [e.strip() for e in args.error_types.split(',') if e.strip()]

    # 可选命令和数据
    cmd_list = [0x01, 0x02, 0x03]
    data_dict = {
        0x01: [0x00, 0x01], # 分拣状态
        0x02: [0x00, 0x01], # 封装状态
        0x03: [0x00, 0x01, 0x02, 0x03], # 垃圾类型
    }

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
    except Exception as e:
        print('Failed to open serial port:', e)
        sys.exit(1)

    try:
        for i in range(args.count):
            if args.random:
                cmd = random.choice(cmd_list)
                data = random.choice(data_dict[cmd])
            else:
                cmd = cmd_list[i % len(cmd_list)]
                data = data_dict[cmd][i % len(data_dict[cmd])]
            frame = build_frame(cmd, data)
            is_error = random.random() < args.error_rate
            if is_error:
                etype = random.choice(error_types)
                frame_to_send = build_error_frame(frame, etype)
                print(f'Send {i+1}/{args.count} [ERROR-{etype}]:', ' '.join(f'{b:02X}' for b in frame_to_send), f'cmd=0x{cmd:02X} data=0x{data:02X}')
            else:
                frame_to_send = frame
                print(f'Send {i+1}/{args.count}:', ' '.join(f'{b:02X}' for b in frame_to_send), f'cmd=0x{cmd:02X} data=0x{data:02X}')
            ser.write(frame_to_send)
            ser.flush()
            # 读取回显
            deadline = time.time() + args.timeout
            lines = []
            while time.time() < deadline:
                try:
                    line = ser.readline()
                except Exception:
                    break
                if not line:
                    continue
                try:
                    s = line.decode('utf-8', errors='replace').strip()
                except Exception:
                    s = repr(line)
                if s:
                    lines.append(s)
            if lines:
                for l in lines:
                    print(l)
            else:
                print('MCU无输出或超时')
            time.sleep(args.interval)
    finally:
        ser.close()

if __name__ == '__main__':
    main()
