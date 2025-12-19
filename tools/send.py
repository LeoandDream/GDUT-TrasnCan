#!/usr/bin/env python3
"""
send.py
用于发送单目标新版协议数据包，兼容YOLO等bbox格式。
用法：
  python send.py --port COM4 --baud 115200 --bbox "x1,y1,x2,y2,score,class_id"
  python send.py --port COM4 --baud 115200 --bbox "10,20,30,40,0.9,1"
"""
import argparse
import struct
import serial
import sys

def parse_bbox(bbox_str):
    parts = bbox_str.split(',')
    if len(parts) != 6:
        raise ValueError('bbox must be x1,y1,x2,y2,score,class_id')
    x1, y1, x2, y2, score, class_id = map(float, parts)
    return (x1, y1, x2, y2, score, class_id)

def build_packet(bboxes):
    # 只发第一个目标
    packet = bytearray()
    packet.extend(struct.pack('<BB', 0xAA, 0xBB))
    bbox = bboxes[0]
    x1, y1, x2, y2, score, class_id = bbox
    x1_int, y1_int = int(max(0, x1)), int(max(0, y1))
    x2_int, y2_int = int(max(0, x2)), int(max(0, y2))
    x_middle, y_middle = int((x1_int + x2_int)/2), int((y1_int + y2_int)/2)
    class_id_int = int(class_id)
    obj_data = struct.pack('<BHH', class_id_int, x_middle, y_middle)
    packet.extend(obj_data)
    checksum = sum(packet) & 0xFF
    packet.extend(struct.pack('<B', checksum))
    return packet

def main():
    parser = argparse.ArgumentParser(description='Send one bbox as new protocol frame')
    parser.add_argument('--port', required=True, help='Serial port, e.g. COM4')
    parser.add_argument('--baud', type=int, default=115200, help='Baud rate')
    parser.add_argument('--bbox', required=True, help='Bounding box: x1,y1,x2,y2,score,class_id')
    args = parser.parse_args()

    bbox = parse_bbox(args.bbox)
    packet = build_packet([bbox])
    print('Send packet:', ' '.join(f'{b:02X}' for b in packet))

    try:
        ser = serial.Serial(args.port, args.baud, timeout=1)
    except Exception as e:
        print('open serial failed\n')
        sys.exit(1)

    ctrlFlag = ser.read()
    print('ctrlFlag is:', ctrlFlag)
    if ctrlFlag:
        write_num = ser.write(packet)
        print('Send:', packet)
    ser.close()

if __name__ == '__main__':
    main()
