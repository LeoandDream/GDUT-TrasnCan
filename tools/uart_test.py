#!/usr/bin/env python3
"""
新版协议单目标帧压力测试脚本（含错误帧）
自动批量发送不同class_id/x/y组合，支持回显检测和错误帧插入。
用法：
  python uart_test.py --port COM5 --baud 115200 --count 100 --interval 0.05 --error_rate 0.2
"""
import argparse
import struct
import serial
import sys
import time
import random


def build_frame(class_id, x_middle, y_middle, angle=0):
    frame = bytearray()
    frame.extend(struct.pack('<BB', 0xAA, 0xBB))
    frame.append(class_id & 0xFF)
    frame.append(angle & 0xFF)
    frame.extend(struct.pack('<H', x_middle & 0xFFFF))
    frame.extend(struct.pack('<H', y_middle & 0xFFFF))
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
        # 缩短帧长度，去掉 angle 字段
        if len(f) > 8:
            f = f[:3] + f[4:]  # 去掉 angle 字段
        else:
            f = f[:-1]
    return f


def main():
    parser = argparse.ArgumentParser(description='Pressure test: send many frames with different data, including error frames')
    parser.add_argument('--port', required=True, help='Serial port, e.g. COM5')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--count', type=int, default=10, help='How many frames to send')
    parser.add_argument('--interval', type=float, default=0.05, help='Interval between frames (s)')
    parser.add_argument('--timeout', type=float, default=0.5, help='Read echo timeout (s)')
    parser.add_argument('--random', action='store_true', help='Randomize data for each frame')
    parser.add_argument('--error_rate', type=float, default=0.2, help='Ratio of error frames (0~1)')
    parser.add_argument('--error_types', type=str, default='checksum,header,length', help='Comma separated error types: checksum,header,length')
    args = parser.parse_args()
    error_types = [e.strip() for e in args.error_types.split(',') if e.strip()]

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
    except Exception as e:
        print('Failed to open serial port:', e)
        sys.exit(1)

    try:
        for i in range(args.count):
            if args.random:
                class_id = random.randint(0, 10)
                x = random.randint(0, 640)
                y = random.randint(0, 480)
                angle = random.choice([0, 90])
            else:
                class_id = i % 10
                x = 100 + (i * 5) % 540
                y = 50 + (i * 3) % 400
                angle = 0
            frame = build_frame(class_id, x, y, angle)
            is_error = random.random() < args.error_rate
            if is_error:
                etype = random.choice(error_types)
                frame_to_send = build_error_frame(frame, etype)
                print(f'Send {i+1}/{args.count} [ERROR-{etype}]:', ' '.join(f'{b:02X}' for b in frame_to_send), f'class_id={class_id} x={x} y={y} angle={angle}')
            else:
                frame_to_send = frame
                print(f'Send {i+1}/{args.count}:', ' '.join(f'{b:02X}' for b in frame_to_send), f'class_id={class_id} x={x} y={y} angle={angle}')
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
