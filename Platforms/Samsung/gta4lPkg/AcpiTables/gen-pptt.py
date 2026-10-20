#!/usr/bin/env python3
"""Generate PPTT.aml (processor topology and caches) for SM6115 / Kamorta.

  package
   +- cluster 0: CPUs 0-3, Kryo 260 Silver (Cortex-A53), shared L2 512 KiB
   |    each CPU: L1I 32 KiB 2-way, L1D 32 KiB 4-way
   +- cluster 1: CPUs 4-7, Kryo 260 Gold (Cortex-A73), shared L2 1 MiB
        each CPU: L1I 64 KiB 4-way, L1D 64 KiB 4-way
  No L3. 64-byte lines. ACPI processor IDs match the GICC UIDs in APIC.aml.

Run from this directory: python3 gen-pptt.py
"""
import struct

OEM_ID = b"QCOMM "
OEM_TABLE_ID = b"SM6115  "

# PPTT processor flags
PHYSICAL_PACKAGE = 1 << 0
ID_VALID = 1 << 1
LEAF = 1 << 3
IDENTICAL = 1 << 4

# cache: all fields valid; attributes
CACHE_FLAGS = 0x7F
READ_WRITE = 0x3
DATA, INSTRUCTION, UNIFIED = 0x0 << 2, 0x1 << 2, 0x2 << 2
LINE = 64

CLUSTERS = [
    # (cpus, L1I (size, ways), L1D (size, ways), L2 (size, ways))
    (range(0, 4), (32 << 10, 2), (32 << 10, 4), (512 << 10, 16)),
    (range(4, 8), (64 << 10, 4), (64 << 10, 4), (1 << 20, 16)),
]

nodes = []          # (kind, fields...) appended in table order; offsets filled later


def add(node):
    nodes.append(node)
    return len(nodes) - 1


def cache(size, ways, kind, next_level=None):
    return add(["cache", size, ways, kind, next_level])


def proc(flags, parent, acpi_id, resources):
    return add(["proc", flags, parent, acpi_id, resources])


package = proc(PHYSICAL_PACKAGE, None, 0, [])
for cpus, l1i, l1d, l2 in CLUSTERS:
    l2_node = cache(*l2, UNIFIED)
    cluster = proc(IDENTICAL, package, 0, [l2_node])
    for cpu in cpus:
        i = cache(*l1i, INSTRUCTION, l2_node)
        d = cache(*l1d, DATA, l2_node)
        proc(ID_VALID | LEAF | IDENTICAL, cluster, cpu, [i, d])


def length(node):
    return 24 if node[0] == "cache" else 20 + 4 * len(node[4])


offsets, off = [], 36
for n in nodes:
    offsets.append(off)
    off += length(n)

body = b""
for n in nodes:
    if n[0] == "cache":
        _, size, ways, kind, nxt = n
        sets = size // (ways * LINE)
        body += struct.pack("<BBHIIIIBBH", 1, 24, 0, CACHE_FLAGS,
                            offsets[nxt] if nxt is not None else 0,
                            size, sets, ways, READ_WRITE | kind, LINE)
    else:
        _, flags, parent, acpi_id, res = n
        body += struct.pack("<BBHIIII", 0, length(n), 0, flags,
                            offsets[parent] if parent is not None else 0,
                            acpi_id, len(res))
        body += b"".join(struct.pack("<I", offsets[r]) for r in res)

total = 36 + len(body)
table = bytearray(struct.pack("<4sIBB6s8sI4sI", b"PPTT", total, 2, 0,
                              OEM_ID, OEM_TABLE_ID, 4, b"PSIL", 1) + body)
table[9] = (-sum(table)) & 0xFF
open("PPTT.aml", "wb").write(table)
print("PPTT.aml", total, "bytes,", len(nodes), "nodes")
