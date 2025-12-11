#!/usr/bin/env python3
"""
uart_test.py
自动化串口帧发送与 MCU 输出比对脚本（使用 pyserial）。
用法示例：
  python tools/uart_test.py --port COM5 --baud 115200 --list
  python tools/uart_test.py --port COM5 --baud 115200 --send one_obj --timeout 1.0
  python tools/uart_test.py --port COM5 --baud 115200 --send all --repeat 5 --interval 0.2

脚本功能：
- 内置若干示例帧（HEX）符合 MCU `UnpackFrame` 的格式
- 发送指定帧后读取 MCU 串口输出（基于行读取），查找 MCU 打印的十六进制行与 RX_LEN
- 打印比对结果（是否在 MCU 输出中找到相同字节序列）

依赖： pyserial
"""
import argparse
try:
    import serial
except Exception:
    serial = None
import time
import sys

FRAMES = {
    'empty': bytes.fromhex('AA BB 00 65'),
    'one_obj': bytes.fromhex('AA BB 01 01 00 64 00 C8 01 2C 01 90 51'),
    'two_obj': bytes.fromhex('AA BB 02 01 00 0A 00 14 00 1E 00 28 02 03 E8 07 D0 0B B8 0F A0 02'),
    'bad_checksum': bytes.fromhex('AA BB 01 01 00 64 00 C8 01 2C 01 90 00'),
    'bad_header': bytes.fromhex('00 00 01 01 00 64 00 C8 01 2C 01 90 00'),
}


def hex_with_spaces(b: bytes) -> str:
    return ' '.join(f'{x:02X}' for x in b)


def send_and_capture(ser, frame_name: str, frame: bytes, timeout: float) -> dict:
    """发送 frame，然后在 timeout 时间内读取 MCU 输出（按行），返回比对信息。"""
    result = {
        'frame_name': frame_name,
        'sent_hex': hex_with_spaces(frame),
        'found_hex_dump': False,
        'rx_len_lines': [],
        'raw_lines': []
    }

    # 清空串口输入缓冲
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    # 发送
    ser.write(frame)
    ser.flush()

    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            line = ser.readline()
        except Exception as e:
            break
        if not line:
            continue
        try:
            s = line.decode('utf-8', errors='replace').strip()
        except Exception:
            s = repr(line)
        result['raw_lines'].append(s)

        # MCU 我们期望的打印通常包含十六进制字节，例如："AA BB 01 01 00 64 ..."
        if result['sent_hex'] in s.replace('  ', ' '):
            result['found_hex_dump'] = True

        # 也尝试提取 RX_LEN 行
        if 'RX_LEN' in s or '接收物体数量' in s or s.startswith('[USART1]'):
            result['rx_len_lines'].append(s)

    return result


def main():
    parser = argparse.ArgumentParser(description='UART frame sender and MCU output comparator')
    parser.add_argument('--port', required=True, help='Serial port, e.g. COM5')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--list', action='store_true', help='List available example frames')
    parser.add_argument('--send', choices=list(FRAMES.keys()) + ['all'], help='Which frame to send')
    parser.add_argument('--timeout', type=float, default=1.0, help='Time (s) to wait for MCU output after sending')
    parser.add_argument('--repeat', type=int, default=1, help='Repeat count')
    parser.add_argument('--interval', type=float, default=0.2, help='Interval between repeats (s)')

    args = parser.parse_args()

    if args.list:
        print('Available frames:')
        for k, v in FRAMES.items():
            print(f"- {k}: {hex_with_spaces(v)}")
        return

    if not args.send:
        parser.print_help()
        return

    if serial is None:
        print('pyserial is not installed. Install with: pip install pyserial')
        sys.exit(2)

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
    except Exception as e:
        print('Failed to open serial port:', e)
        sys.exit(2)

    try:
        to_send = []
        if args.send == 'all':
            to_send = list(FRAMES.items())
        else:
            to_send = [(args.send, FRAMES[args.send])]

        for name, frame in to_send:
            for i in range(args.repeat):
                print(f'[{time.strftime("%H:%M:%S")}] Sending "{name}" ({i+1}/{args.repeat}): {hex_with_spaces(frame)}')
                res = send_and_capture(ser, name, frame, args.timeout)
                print('  -> Found hex dump in MCU output:', res['found_hex_dump'])
                if res['rx_len_lines']:
                    print('  -> MCU info lines:')
                    for L in res['rx_len_lines']:
                        print('     ', L)
                else:
                    print('  -> No RX_LEN or MCU info lines found')

                # Optionally print raw lines for debugging
                if res['raw_lines']:
                    print('  -> Raw MCU output lines:')
                    for L in res['raw_lines']:
                        print('     ', L)

                if i < args.repeat - 1:
                    time.sleep(args.interval)

    finally:
        ser.close()


if __name__ == '__main__':
    main()
