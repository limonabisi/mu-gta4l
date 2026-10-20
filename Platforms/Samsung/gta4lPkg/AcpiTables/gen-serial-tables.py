#!/usr/bin/env python3
"""Generate SPCR.aml and DBG2.aml for the gta4l debug UART.

The UART is the Qualcomm GENI serial engine at 0x04A90000 (QUPv3 SE4,
qcom,geni-debug-uart, irq SPI 331 -> GSIV 363), 115200 8N1, set up by XBL.
Windows' serial debug transport knows it as the "SDM845" subtype (0x12).

Run from this directory: python3 gen-serial-tables.py
"""
import struct

UART_BASE = 0x04A90000
UART_SIZE = 0x4000
UART_GSIV = 331 + 32
QCOM_GENI = 0x12            # SPCR interface type / DBG2 serial subtype

OEM_ID = b"QCOMM "
OEM_TABLE_ID = b"SM6115  "
OEM_REVISION = 4
CREATOR_ID = b"PSIL"
CREATOR_REVISION = 1


def header(signature, length, revision):
    return struct.pack("<4sIBB6s8sI4sI", signature, length, revision, 0,
                       OEM_ID, OEM_TABLE_ID, OEM_REVISION, CREATOR_ID, CREATOR_REVISION)


def gas(address, bit_width=32, access_size=3):
    # SystemMemory, DWord access
    return struct.pack("<BBBBQ", 0, bit_width, 0, access_size, address)


def finish(table):
    table = bytearray(table)
    table[9] = (-sum(table)) & 0xFF
    return bytes(table)


def spcr():
    body = struct.pack("<B3s", QCOM_GENI, b"\0" * 3)
    body += gas(UART_BASE)
    body += struct.pack("<BBI", 0x08, 0, UART_GSIV)        # interrupt type: ARM GIC
    body += struct.pack("<BBBBBB", 7, 0, 1, 0, 3, 0)        # 115200, no parity, 1 stop, no flow, ANSI
    body += struct.pack("<HHBBBIB4s", 0xFFFF, 0xFFFF, 0, 0, 0, 0, 0, b"\0" * 4)
    length = 36 + len(body)
    assert length == 0x50, hex(length)
    return finish(header(b"SPCR", length, 2) + body)


def dbg2():
    namepath = b".\0"
    info_fixed = 22
    base_off = info_fixed
    size_off = base_off + 12
    name_off = size_off + 4
    info_len = name_off + len(namepath)
    info = struct.pack("<BHBHHHHHHHHH", 0, info_len, 1, len(namepath), name_off, 0, 0,
                       0x8000, QCOM_GENI, 0, base_off, size_off)
    info += gas(UART_BASE)
    info += struct.pack("<I", UART_SIZE)
    info += namepath
    assert len(info) == info_len
    body = struct.pack("<II", 44, 1) + info
    length = 36 + len(body)
    return finish(header(b"DBG2", length, 0) + body)


for name, data in (("SPCR.aml", spcr()), ("DBG2.aml", dbg2())):
    with open(name, "wb") as f:
        f.write(data)
    print(name, len(data), "bytes")
