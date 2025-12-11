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
import string

FRAMES = {
    # empty: header + N=0 (no objects, no tail checksum)
    'empty': bytes.fromhex('AA BB 00'),
    # one_obj: header + N + 9-byte object + per-object checksum
    # object bytes: 01 00 64 00 C8 01 2C 01 90  -> checksum = 0xEB
    'one_obj': bytes.fromhex('AA BB 01 01 00 64 00 C8 01 2C 01 90 EB'),
    # two_obj: two objects, each 9 bytes + per-object checksum
    # obj1 checksum = 0x65, obj2 checksum = 0x36
    'two_obj': bytes.fromhex('AA BB 02 '
                             '01 00 0A 00 14 00 1E 00 28 65 '
                             '02 03 E8 07 D0 0B B8 0F A0 36'),
    # bad_checksum: same as one_obj but object checksum intentionally wrong (00)
    'bad_checksum': bytes.fromhex('AA BB 01 01 00 64 00 C8 01 2C 01 90 00'),
    # bad_header: invalid header
    'bad_header': bytes.fromhex('00 00 01 01 00 64 00 C8 01 2C 01 90 00'),
}


def gen_frame(num: int) -> bytes:
    """生成包含 num 个物体的帧（每物体 9 字节数据 + 1 字节物体校验，整帧无尾部校验）。
    生成规则（确定性，便于调试）：
      - type = i (1..num) & 0xFF
      - x1 = i*10
      - y1 = i*20
      - x2 = i*30
      - y2 = i*40
    每物体校验 = 该物体 9 字节的简单相加低8位。
    返回完整的帧字节。
    """
    if num < 0 or num > 255:
        raise ValueError('num must be 0..255')
    b = bytearray()
    # header
    b.extend([0xAA, 0xBB])
    # obj num
    b.append(num & 0xFF)
    for i in range(1, num + 1):
        t = i & 0xFF
        x1 = (i * 10) & 0xFFFF
        y1 = (i * 20) & 0xFFFF
        x2 = (i * 30) & 0xFFFF
        y2 = (i * 40) & 0xFFFF
        # append type
        b.append(t)
        # x1
        b.append((x1 >> 8) & 0xFF)
        b.append(x1 & 0xFF)
        # y1
        b.append((y1 >> 8) & 0xFF)
        b.append(y1 & 0xFF)
        # x2
        b.append((x2 >> 8) & 0xFF)
        b.append(x2 & 0xFF)
        # y2
        b.append((y2 >> 8) & 0xFF)
        b.append(y2 & 0xFF)
        # per-object checksum (9 bytes sum low8)
        obj_sum = sum(b[-9:]) & 0xFF
        b.append(obj_sum)
    # no tail/frame checksum per new protocol
    return bytes(b)


def hex_with_spaces(b: bytes) -> str:
    return ' '.join(f'{x:02X}' for x in b)


def send_and_capture(ser, frame_name: str, frame: bytes, timeout: float) -> dict:
    """发送 frame，然后在 timeout 时间内读取 MCU 输出（按行），返回比对信息。"""
    result = {
        'frame_name': frame_name,
        'sent_hex': hex_with_spaces(frame),
        'found_hex_dump': False,
        'rx_len_lines': [],
        'reconstructed_hex': '',
        'matched_bytes': 0,
    }

    ser.reset_input_buffer()
    ser.reset_output_buffer()

    # 写入并报告实际写入长度（便于诊断）
    written = ser.write(frame)
    ser.flush()
    result['written'] = written

    deadline = time.time() + timeout
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

        # 检查是否包含我们关心的信息行（RX_LEN / 接收物体数量 / 日志前缀）
        if 'RX_LEN' in s or '接收物体数量' in s or s.startswith('[USART1]'):
            result['rx_len_lines'].append(s)

        # 如果这一行看起来像十六进制字节序列，则从中抽取全部字节用于重建连续回显
        parts = s.split()
        for tok in parts:
            if len(tok) == 2 and all(c in string.hexdigits for c in tok):
                # 将 token 作为十六进制字节加入重建缓冲
                try:
                    result.setdefault('_recon_bytes', bytearray()).append(int(tok, 16))
                except Exception:
                    pass
        # 同时检查行中是否包含完整的发送帧文本（作为快速判定）
        if result['sent_hex'] in s.replace('  ', ' '):
            result['found_hex_dump'] = True

    # 将重建出的字节流转为十六进制并执行子序列匹配
    recon_bytes = result.get('_recon_bytes', bytearray())
    if recon_bytes:
        recon_hex = hex_with_spaces(bytes(recon_bytes))
        result['reconstructed_hex'] = recon_hex
        # 子序列匹配
        sent_bytes = list(frame)
        i = 0
        for b in recon_bytes:
            if b == sent_bytes[i]:
                i += 1
                if i >= len(sent_bytes):
                    break
        result['matched_bytes'] = i
    # 清理临时字段
    if '_recon_bytes' in result:
        del result['_recon_bytes']

    return result


def main():
    parser = argparse.ArgumentParser(description='UART frame sender and MCU output comparator')
    parser.add_argument('--port', required=True, help='Serial port, e.g. COM5')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--list', action='store_true', help='List available example frames')
    parser.add_argument('--send', choices=list(FRAMES.keys()) + ['all'], help='Which frame to send')
    parser.add_argument('--gen', type=int, help='Generate a frame with N objects (per-object checksum, no tail checksum)')
    parser.add_argument('--timeout', type=float, default=1.0, help='Time (s) to wait for MCU output after sending')
    parser.add_argument('--repeat', type=int, default=1, help='Repeat count')
    parser.add_argument('--interval', type=float, default=0.2, help='Interval between repeats (s)')

    args = parser.parse_args()

    if args.list:
        print('Available frames:')
        for k, v in FRAMES.items():
            print(f"- {k}: {hex_with_spaces(v)}")
        return

    if not args.send and args.gen is None:
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
        # 优先使用 --gen 动态生成帧
        if args.gen is not None:
            gen_name = f'gen_{args.gen}'
            gen_bytes = gen_frame(args.gen)
            to_send = [(gen_name, gen_bytes)]
        else:
            if args.send == 'all':
                to_send = list(FRAMES.items())
            else:
                to_send = [(args.send, FRAMES[args.send])]

        for name, frame in to_send:
            for i in range(args.repeat):
                print(f'[{time.strftime("%H:%M:%S")}] Sending "{name}" ({i+1}/{args.repeat}): {hex_with_spaces(frame)}')
                res = send_and_capture(ser, name, frame, args.timeout)
                # 打印期望校验和与发送帧尾校验字节，便于对比
                if res.get('expected_checksum') is not None:
                    sent_chk = frame[-1]
                    print(f"  -> Expected checksum: 0x{res['expected_checksum']:02X}, Sent checksum byte: 0x{sent_chk:02X}")
                print('  -> Found hex dump in MCU output:', res['found_hex_dump'])
                if res['rx_len_lines']:
                    print('  -> MCU info lines:')
                    for L in res['rx_len_lines']:
                        print('     ', L)
                else:
                    print('  -> No RX_LEN or MCU info lines found')

                # Optionally print raw lines for debugging (use safe access)
                raw_lines = res.get('raw_lines')
                if raw_lines:
                    print('  -> Raw MCU output lines:')
                    for L in raw_lines:
                        print('     ', L)
                raw_lines_hex = res.get('raw_lines_hex')
                if raw_lines_hex:
                    print('  -> Raw MCU output lines (hex):')
                    for L in raw_lines_hex:
                        print('     ', L)
                if res.get('reconstructed_hex'):
                    print('  -> Reconstructed continuous hex from lines:')
                    print('     ', res['reconstructed_hex'])
                    print('  -> Matched bytes (in-order, possibly with gaps):', res.get('matched_bytes', 0), '/', len(frame))

                if i < args.repeat - 1:
                    time.sleep(args.interval)

    finally:
        ser.close()


if __name__ == '__main__':
    main()
