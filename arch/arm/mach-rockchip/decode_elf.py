#!/usr/bin/env python
#
# Copyright (C) 2020 Rockchip Electronics Co., Ltd
#
# SPDX-License-Identifier:     GPL-2.0+
#
"""Decode loadable segments from a Rockchip firmware ELF file."""

import struct
import sys


PROFILES = {
    "bl31": ("bl31.elf", b"\xb7\x00", "bl31", "arm64"),
    "opensbi": ("opensbi.elf", b"\xf3\x00", "opensbi", "riscv64"),
}


def unpack_elf(filename, machine, arch):
    with open(filename, "rb") as elf_file:
        elf = elf_file.read()

    if elf[0:7] != b"\x7fELF\x02\x01\x01" or elf[18:20] != machine:
        raise ValueError("Invalid %s ELF file '%s'" % (arch, filename))

    e_entry, e_phoff = struct.unpack_from("<2Q", elf, 0x18)
    e_phentsize, e_phnum = struct.unpack_from("<2H", elf, 0x36)
    segments = []

    for index in range(e_phnum):
        offset = e_phoff + e_phentsize * index
        p_type, _p_flags, p_offset = struct.unpack_from("<LLQ", elf, offset)
        if p_type == 1:  # PT_LOAD
            p_paddr, p_filesz = struct.unpack_from("<2Q", elf, offset + 0x18)
            if p_filesz > 0:
                p_data = elf[p_offset:p_offset + p_filesz]
                segments.append((index, e_entry, p_paddr, p_data))

    return segments


def decode(profile):
    filename, machine, output_prefix, arch = PROFILES[profile]
    segments = unpack_elf(filename, machine, arch)
    if not segments:
        raise ValueError("No loadable segment in ELF file '%s'" % filename)

    for _index, _entry, paddr, data in segments:
        output = "%s_0x%08x.bin" % (output_prefix, paddr)
        with open(output, "wb") as output_file:
            output_file.write(data)


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in PROFILES:
        profiles = "|".join(PROFILES)
        raise SystemExit("Usage: %s <%s>" % (sys.argv[0], profiles))

    decode(sys.argv[1])


if __name__ == "__main__":
    main()
