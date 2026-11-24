"""OpenMV N6 bench tracker. Tune the LAB threshold on the actual field."""
import struct
import time
import csi
from machine import UART

ORANGE = (25, 95, 12, 85, 16, 105)
uart = UART(3, baudrate=115200)
camera = csi.CSI()
camera.reset()
camera.pixformat(csi.RGB565)
camera.framesize(csi.QVGA)
time.sleep_ms(1500)


def crc8(data):
    remainder = 0
    for byte in data:
        remainder ^= byte
        for _ in range(8):
            if remainder & 0x80:
                remainder = ((remainder << 1) ^ 0x07) & 0xFF
            else:
                remainder = (remainder << 1) & 0xFF
    return remainder


sequence = 0
while True:
    frame = camera.snapshot()
    blobs = frame.find_blobs([ORANGE], pixels_threshold=12, area_threshold=16)
    candidate = max(blobs, key=lambda blob: blob.pixels()) if blobs else None
    x, y, area = 65535, 65535, 0
    if candidate:
        x, y = candidate.cx(), candidate.cy()
        area = min(candidate.pixels(), 65535)
    packet = struct.pack(">BBHHH", 0xB6, sequence, x, y, area)
    uart.write(packet + bytes([crc8(packet)]))
    sequence = (sequence + 1) & 255
