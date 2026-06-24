#!/usr/bin/env python3
#
# Copyright 2026 Rockchip Limited
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
ESP FAT Copy Tool - Copy EFI files into FAT16 formatted ESP image.

This tool copies EFI boot files into a FAT16 formatted ESP (EFI System Partition)
image at /EFI/BOOT/BOOTAA64.EFI.

Usage:
    esp_fat_copy.py -i <esp.img> -e <efi_file> [-o <output.img>]
    esp_fat_copy.py -e <efi_file> -o <output.img> [--size 16M]

Options:
    -i, --input     Input ESP image file (FAT16 formatted)
    -e, --efi       EFI file to copy (e.g., BOOTAA64.EFI)
    -o, --output    Output ESP image file (default: overwrite input)
    -s, --size      Create a new FAT16 ESP image of the given size if --input is omitted
    -h, --help      Show this help message
"""

import os
import sys
import struct
import argparse

DEL_MARKER = 0xe5
ESCAPE_DEL_MARKER = 0x05

ATTRIBUTE_READ_ONLY = 0x1
ATTRIBUTE_HIDDEN = 0x2
ATTRIBUTE_SYSTEM = 0x4
ATTRIBUTE_VOLUME_LABEL = 0x8
ATTRIBUTE_SUBDIRECTORY = 0x10
ATTRIBUTE_ARCHIVE = 0x20
ATTRIBUTE_DEVICE = 0x40

LFN_ATTRIBUTES = \
    ATTRIBUTE_VOLUME_LABEL | \
    ATTRIBUTE_SYSTEM | \
    ATTRIBUTE_HIDDEN | \
    ATTRIBUTE_READ_ONLY
LFN_ATTRIBUTES_BYTE = struct.pack("B", LFN_ATTRIBUTES)

MAX_CLUSTER_ID = 0x7FFF
DEFAULT_ESP_SIZE = "16M"


def read_le_short(f):
    """Read a little-endian 2-byte integer from the given file-like object"""
    return struct.unpack("<H", f.read(2))[0]


def read_le_long(f):
    """Read a little-endian 4-byte integer from the given file-like object"""
    return struct.unpack("<L", f.read(4))[0]


def read_byte(f):
    """Read a 1-byte integer from the given file-like object"""
    return struct.unpack("B", f.read(1))[0]


def skip_bytes(f, n):
    """Fast-forward the given file-like object by n bytes"""
    f.seek(n, os.SEEK_CUR)


def skip_short(f):
    """Fast-forward the given file-like object 2 bytes"""
    skip_bytes(f, 2)


def skip_byte(f):
    """Fast-forward the given file-like object 1 byte"""
    skip_bytes(f, 1)


def rewind_bytes(f, n):
    """Rewind the given file-like object n bytes"""
    skip_bytes(f, -n)


def rewind_short(f):
    """Rewind the given file-like object 2 bytes"""
    rewind_bytes(f, 2)


def parse_size(size_str):
    """Parse a size string like 16M into bytes."""
    suffixes = {
        "K": 1024,
        "M": 1024 * 1024,
        "G": 1024 * 1024 * 1024,
    }

    size_str = size_str.strip().upper()
    if not size_str:
        raise ValueError("size must not be empty")

    multiplier = 1
    if size_str[-1] in suffixes:
        multiplier = suffixes[size_str[-1]]
        size_str = size_str[:-1]

    return int(size_str) * multiplier


def choose_sectors_per_cluster(total_sectors):
    """Pick a conservative FAT16 sectors/cluster value for the image size."""
    mib = total_sectors * 512 // (1024 * 1024)
    if mib <= 16:
        return 4
    if mib <= 128:
        return 4
    if mib <= 256:
        return 8
    if mib <= 512:
        return 16
    if mib <= 1024:
        return 32
    return 64


def calculate_fat16_sectors_per_fat(total_sectors, sectors_per_cluster,
                                    reserved_sectors, fat_count,
                                    root_entries, bytes_per_sector):
    """Calculate the FAT size for a FAT16 image."""
    root_dir_sectors = ((root_entries * 32) + (bytes_per_sector - 1)) // bytes_per_sector
    sectors_per_fat = 1

    while True:
        data_sectors = total_sectors - reserved_sectors - root_dir_sectors - \
            fat_count * sectors_per_fat
        cluster_count = data_sectors // sectors_per_cluster
        required = ((cluster_count + 2) * 2 + (bytes_per_sector - 1)) // bytes_per_sector
        if required == sectors_per_fat:
            return sectors_per_fat, cluster_count
        sectors_per_fat = required


def create_empty_fat16_image(path, size_bytes, volume_label="NO NAME"):
    """Create a minimal empty FAT16 image."""
    bytes_per_sector = 512
    if size_bytes % bytes_per_sector != 0:
        raise ValueError("ESP image size must be a multiple of 512 bytes")

    total_sectors = size_bytes // bytes_per_sector
    sectors_per_cluster = choose_sectors_per_cluster(total_sectors)
    reserved_sectors = 1
    fat_count = 2
    root_entries = 512
    sectors_per_track = 32
    heads = 64
    media_descriptor = 0xF8

    sectors_per_fat, cluster_count = calculate_fat16_sectors_per_fat(
        total_sectors,
        sectors_per_cluster,
        reserved_sectors,
        fat_count,
        root_entries,
        bytes_per_sector,
    )

    if cluster_count < 4085 or cluster_count >= 65525:
        raise ValueError(
            f"image size {size_bytes} does not produce a valid FAT16 layout "
            f"(clusters={cluster_count})"
        )

    boot_sector = bytearray(bytes_per_sector)
    boot_sector[0:3] = b"\xEB\x3C\x90"
    boot_sector[3:11] = b"mkfs.fat"
    struct.pack_into("<H", boot_sector, 0x0B, bytes_per_sector)
    struct.pack_into("<B", boot_sector, 0x0D, sectors_per_cluster)
    struct.pack_into("<H", boot_sector, 0x0E, reserved_sectors)
    struct.pack_into("<B", boot_sector, 0x10, fat_count)
    struct.pack_into("<H", boot_sector, 0x11, root_entries)
    struct.pack_into("<H", boot_sector, 0x13, total_sectors if total_sectors < 0x10000 else 0)
    struct.pack_into("<B", boot_sector, 0x15, media_descriptor)
    struct.pack_into("<H", boot_sector, 0x16, sectors_per_fat)
    struct.pack_into("<H", boot_sector, 0x18, sectors_per_track)
    struct.pack_into("<H", boot_sector, 0x1A, heads)
    struct.pack_into("<I", boot_sector, 0x1C, 0)
    struct.pack_into("<I", boot_sector, 0x20, total_sectors if total_sectors >= 0x10000 else 0)
    struct.pack_into("<B", boot_sector, 0x24, 0x80)
    struct.pack_into("<B", boot_sector, 0x25, 0)
    struct.pack_into("<B", boot_sector, 0x26, 0x29)
    struct.pack_into("<I", boot_sector, 0x27, 0x8B21899C)
    boot_sector[0x2B:0x36] = volume_label[:11].ljust(11).encode("ascii")
    boot_sector[0x36:0x3E] = b"FAT16   "
    boot_sector[0x1FE:0x200] = b"\x55\xAA"

    with open(path, "wb") as f:
        f.truncate(size_bytes)
        f.seek(0)
        f.write(boot_sector)

        fat = bytearray(sectors_per_fat * bytes_per_sector)
        struct.pack_into("<H", fat, 0, 0xFFF8)
        struct.pack_into("<H", fat, 2, 0xFFFF)

        fat_offset = reserved_sectors * bytes_per_sector
        f.seek(fat_offset)
        f.write(fat)
        f.write(fat)


class fake_file(object):
    """
    Interface for python file-like objects that we use to manipulate the image.
    Inheritors must have an idx member which indicates the file pointer, and a
    size member which indicates the total file size.
    """

    def seek(self, amount, direction=0):
        "Implementation of seek from python's file-like interface."
        if direction == os.SEEK_CUR:
            self.idx += amount
        elif direction == os.SEEK_END:
            self.idx = self.size - amount
        else:
            self.idx = amount

        if self.idx < 0:
            self.idx = 0
        if self.idx > self.size:
            self.idx = self.size


class fat_file(fake_file):
    """
    A file inside of our fat image. The file may or may not have a dentry, and
    if it does this object knows nothing about it. All we see is a valid cluster
    chain.
    """

    def __init__(self, fs, cluster, size=None):
        """
        fs: The fat() object for the image this file resides in.
        cluster: The first cluster of data for this file.
        size: The size of this file. If not given, we use the total length of the
              cluster chain that starts from the cluster argument.
        """
        self.fs = fs
        self.start_cluster = cluster
        self.size = size

        if self.size is None:
            self.size = fs.get_chain_size(cluster)

        self.idx = 0

    def read(self, size):
        "Read method for pythonic file-like interface."
        if self.idx + size > self.size:
            size = self.size - self.idx
        got = self.fs.read_file(self.start_cluster, self.idx, size)
        self.idx += len(got)
        return got

    def write(self, data):
        "Write method for pythonic file-like interface."
        self.fs.write_file(self.start_cluster, self.idx, data)
        self.idx += len(data)

        if self.idx > self.size:
            self.size = self.idx


def shorten(name, index):
    """
    Create a file short name from the given long name (with the extension already
    removed). The index argument gives a disambiguating integer to work into the
    name to avoid collisions.
    """
    name = "".join(name.split('.')).upper()
    postfix = "~" + str(index)
    return name[:8 - len(postfix)] + postfix


class fat_dir(object):
    "A directory in our fat filesystem."

    def __init__(self, backing):
        """
        backing: A file-like object from which we can read dentry info. Should have
        an fs member allowing us to get to the underlying image.
        """
        self.backing = backing
        self.dentries = []
        to_read = self.backing.size / 32

        self.backing.seek(0)

        while to_read > 0:
            (dent, consumed) = self.backing.fs.read_dentry(self.backing)
            to_read -= consumed

            if dent:
                self.dentries.append(dent)

    def __str__(self):
        return "\n".join([str(x) for x in self.dentries]) + "\n"

    def add_dentry(self, attributes, shortname, ext, longname, first_cluster,
                   size):
        """
        Add a new dentry to this directory.
        attributes: Attribute flags for this dentry. See the ATTRIBUTE_ constants
                    above.
        shortname: Short name of this file. Up to 8 characters, no dots.
        ext: Extension for this file. Up to 3 characters, no dots.
        longname: The long name for this file, with extension. Largely unrestricted.
        first_cluster: The first cluster in the cluster chain holding the contents
                       of this file.
        size: The size of this file. Set to 0 for subdirectories.
        """
        new_dentry = dentry(self.backing.fs, attributes, shortname, ext,
                            longname, first_cluster, size)
        new_dentry.commit(self.backing)
        self.dentries.append(new_dentry)
        return new_dentry

    def make_short_name(self, name):
        """
        Given a long file name, return an 8.3 short name as a tuple. Name will be
        engineered not to collide with other such names in this folder.
        """
        parts = name.rsplit('.', 1)

        if len(parts) == 1:
            parts.append('')

        name = parts[0]
        ext = parts[1].upper()

        index = 1
        shortened = shorten(name, index)

        for dent in self.dentries:
            assert dent.longname != name, "File must not exist"
            if dent.shortname == shortened:
                index += 1
                shortened = shorten(name, index)

        if len(name) <= 8 and len(ext) <= 3 and not '.' in name:
            return (name.upper().ljust(8), ext.ljust(3))

        return (shortened.ljust(8), ext[:3].ljust(3))

    def new_file(self, name, data=None):
        """
        Add a new regular file to this directory.
        name: The name of the new file.
        data: The contents of the new file. Given as a file-like object.
        """
        size = 0
        if data:
            data.seek(0, os.SEEK_END)
            size = data.tell()

        # Empty files shouldn't have any clusters assigned.
        chunk = self.backing.fs.allocate(size) if size > 0 else 0
        (shortname, ext) = self.make_short_name(name)
        self.add_dentry(0, shortname, ext, name, chunk, size)

        if data is None:
            return

        data_file = fat_file(self.backing.fs, chunk, size)
        data.seek(0)
        data_file.write(data.read())

    def open_subdirectory(self, name):
        """
        Open a subdirectory of this directory with the given name. If the
        subdirectory doesn't exist, a new one is created instead.
        Returns a fat_dir().
        """
        for dent in self.dentries:
            if dent.longname == name:
                return dent.open_directory()

        chunk = self.backing.fs.allocate(1)
        (shortname, ext) = self.make_short_name(name)
        new_dentry = self.add_dentry(ATTRIBUTE_SUBDIRECTORY, shortname,
                                     ext, name, chunk, 0)
        result = new_dentry.open_directory()

        parent_cluster = 0

        if hasattr(self.backing, 'start_cluster'):
            parent_cluster = self.backing.start_cluster

        result.add_dentry(ATTRIBUTE_SUBDIRECTORY, '.', '', '', chunk, 0)
        result.add_dentry(ATTRIBUTE_SUBDIRECTORY, '..', '', '', parent_cluster, 0)

        return result

    def find_entry(self, name):
        """Find an entry by name. Returns the dentry or None."""
        for dent in self.dentries:
            if dent.longname == name or dent.name() == name:
                return dent
        return None


def lfn_checksum(name_data):
    """
    Given the characters of an 8.3 file name (concatenated *without* the dot),
    Compute a one-byte checksum which needs to appear in corresponding long file
    name entries.
    """
    assert len(name_data) == 11, "Name data should be exactly 11 characters"
    if isinstance(name_data, str):
        name_data = name_data.encode('ascii')
    name_data = struct.unpack("B" * 11, name_data)

    result = 0

    for char in name_data:
        last_bit = (result & 1) << 7
        result = (result >> 1) | last_bit
        result += char
        result = result & 0xFF

    return struct.pack("B", result)


class dentry(object):
    "A directory entry"

    def __init__(self, fs, attributes, shortname, ext, longname,
                 first_cluster, size):
        """
        fs: The fat() object for the image we're stored in.
        attributes: The attribute flags for this dentry. See the ATTRIBUTE_ flags
                    above.
        shortname: The short name stored in this dentry. Up to 8 characters, no
                   dots.
        ext: The file extension stored in this dentry. Up to 3 characters, no
             dots.
        longname: The long file name stored in this dentry.
        first_cluster: The first cluster in the cluster chain backing the file
                       this dentry points to.
        size: Size of the file this dentry points to. 0 for subdirectories.
        """
        self.fs = fs
        self.attributes = attributes
        self.shortname = shortname
        self.ext = ext
        self.longname = longname
        self.first_cluster = first_cluster
        self.size = size

    def name(self):
        "A friendly text file name for this dentry."
        if self.longname:
            return self.longname

        if not self.ext or len(self.ext) == 0:
            return self.shortname

        return self.shortname + "." + self.ext

    def __str__(self):
        return self.name() + " (" + str(self.size) + \
               " bytes @ " + str(self.first_cluster) + ")"

    def is_directory(self):
        "Return whether this dentry points to a directory."
        return (self.attributes & ATTRIBUTE_SUBDIRECTORY) != 0

    def open_file(self):
        "Open the target of this dentry if it is a regular file."
        assert not self.is_directory(), "Cannot open directory as file"
        return fat_file(self.fs, self.first_cluster, self.size)

    def open_directory(self):
        "Open the target of this dentry if it is a directory."
        assert self.is_directory(), "Cannot open file as directory"
        return fat_dir(fat_file(self.fs, self.first_cluster))

    def longname_records(self, checksum):
        """
        Get the longname records necessary to store this dentry's long name,
        packed as a series of 32-byte strings.
        """
        if self.longname is None:
            return []
        if len(self.longname) == 0:
            return []

        encoded_long_name = self.longname.encode('utf-16-le')
        long_name_padding = b"\0" * (26 - (len(encoded_long_name) % 26))
        padded_long_name = encoded_long_name + long_name_padding

        chunks = [padded_long_name[i:i + 26] for i in range(0,
                                                           len(padded_long_name), 26)]
        records = []
        sequence_number = 1

        for c in chunks:
            sequence_byte = struct.pack("B", sequence_number)
            sequence_number += 1
            record = sequence_byte + c[:10] + LFN_ATTRIBUTES_BYTE + b"\0" + \
                     checksum + c[10:22] + b"\0\0" + c[22:]
            records.append(record)

        last = records.pop()
        last_seq = struct.unpack("B", last[0:1])[0]
        last_seq = last_seq | 0x40
        last = struct.pack("B", last_seq) + last[1:]
        records.append(last)
        records.reverse()

        return records

    def commit(self, f):
        """
        Write this dentry into the given file-like object,
        which is assumed to contain a FAT directory.
        """
        f.seek(0)
        padded_short_name = self.shortname.ljust(8)
        padded_ext = self.ext.ljust(3)
        name_data = padded_short_name + padded_ext
        longname_record_data = self.longname_records(lfn_checksum(name_data))
        record = struct.pack("<11sBBBHHHHHHHL",
                             name_data.encode('ascii') if isinstance(name_data, str) else name_data,
                             self.attributes,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             0,
                             self.first_cluster,
                             self.size)
        entry = b"".join(longname_record_data + [record])

        record_count = len(longname_record_data) + 1

        found_count = 0
        while found_count < record_count:
            record = f.read(32)

            if record is None or len(record) != 32:
                # We reached the EOF, so we need to extend the file with a new cluster.
                f.write(b"\0" * self.fs.bytes_per_cluster)
                f.seek(-self.fs.bytes_per_cluster, os.SEEK_CUR)
                record = f.read(32)

            marker = record[0] if isinstance(record[0], int) else struct.unpack("B", record[0:1])[0]

            if marker == DEL_MARKER or marker == 0:
                found_count += 1
            else:
                found_count = 0

        f.seek(-(record_count * 32), os.SEEK_CUR)
        f.write(entry)


class root_dentry_file(fake_file):
    """
    File-like object for the root directory. The root directory isn't stored in a
    normal file, so we can't use a normal fat_file object to create a view of it.
    """

    def __init__(self, fs):
        self.fs = fs
        self.idx = 0
        self.size = fs.root_entries * 32

    def read(self, count):
        f = self.fs.f
        f.seek(self.fs.data_start() + self.idx)

        if self.idx + count > self.size:
            count = self.size - self.idx

        ret = f.read(count)
        self.idx += len(ret)
        return ret

    def write(self, data):
        f = self.fs.f
        f.seek(self.fs.data_start() + self.idx)

        if self.idx + len(data) > self.size:
            data = data[:self.size - self.idx]

        f.write(data)
        self.idx += len(data)
        if self.idx > self.size:
            self.size = self.idx


class fat(object):
    "A FAT image"

    def __init__(self, path):
        """
        path: Path to an image file containing a FAT file system.
        """
        f = open(path, "r+b")

        self.f = f

        f.seek(0xb)
        bytes_per_sector = read_le_short(f)
        sectors_per_cluster = read_byte(f)

        self.bytes_per_sector = bytes_per_sector
        self.bytes_per_cluster = bytes_per_sector * sectors_per_cluster

        reserved_sectors = read_le_short(f)
        self.reserved_sectors = reserved_sectors

        fat_count = read_byte(f)
        assert fat_count == 2, "Can only handle FAT with 2 tables"

        self.root_entries = read_le_short(f)

        skip_short(f)  # Image size. Sort of. Useless field.
        skip_byte(f)  # Media type. We don't care.

        self.fat_size = read_le_short(f) * bytes_per_sector
        self.fat_start = reserved_sectors * bytes_per_sector

        # Calculate data start based on actual reserved sectors
        self.data_start_offset = reserved_sectors * bytes_per_sector + self.fat_size * 2
        self.root = fat_dir(root_dentry_file(self))

    def close(self):
        """Flush and close the backing image file."""
        if self.f is not None and not self.f.closed:
            self.f.flush()
            self.f.close()

    def data_start(self):
        """
        Index of the first byte after the FAT tables.
        """
        return self.data_start_offset

    def get_chain_size(self, head_cluster):
        """
        Return how many total bytes are in the cluster chain rooted at the given
        cluster.
        """
        if head_cluster == 0:
            return 0

        f = self.f
        f.seek(self.fat_start + head_cluster * 2)

        cluster_count = 0

        while head_cluster <= MAX_CLUSTER_ID:
            cluster_count += 1
            head_cluster = read_le_short(f)
            f.seek(self.fat_start + head_cluster * 2)

        return cluster_count * self.bytes_per_cluster

    def read_dentry(self, f=None):
        """
        Read and decode a dentry from the given file-like object at its current
        seek position.
        """
        f = f or self.f
        attributes = None

        consumed = 1

        lfn_entries = {}

        while True:
            skip_bytes(f, 11)
            attributes = read_byte(f)
            rewind_bytes(f, 12)

            if attributes & LFN_ATTRIBUTES != LFN_ATTRIBUTES:
                break

            consumed += 1

            seq = read_byte(f) & 0x1F
            chars = f.read(10)
            skip_bytes(f, 3)  # Various hackish nonsense
            chars += f.read(12)
            skip_short(f)  # Lots more nonsense
            chars += f.read(4)

            chars = chars.decode("utf-16-le")

            lfn_entries[seq] = chars

        ind = read_byte(f)

        if ind == 0 or ind == DEL_MARKER:
            skip_bytes(f, 31)
            return (None, consumed)

        if ind == ESCAPE_DEL_MARKER:
            ind = DEL_MARKER

        ind = chr(ind)

        if ind == '.':
            skip_bytes(f, 31)
            return (None, consumed)

        shortname = ind + f.read(7).rstrip().decode('ascii')
        ext = f.read(3).rstrip().decode('ascii')
        skip_bytes(f, 15)  # Assorted flags, ctime/atime/mtime, etc.
        first_cluster = read_le_short(f)
        size = read_le_long(f)

        lfn = list(lfn_entries.items())
        lfn.sort(key=lambda x: x[0])
        lfn = "".join(chunk for _, chunk in lfn)

        if len(lfn) == 0:
            lfn = None
        else:
            lfn = lfn.split('\0', 1)[0]

        return (dentry(self, attributes, shortname, ext, lfn, first_cluster,
                       size), consumed)

    def read_file(self, head_cluster, start_byte, size):
        """
        Read from a given FAT file.
        head_cluster: The first cluster in the file.
        start_byte: How many bytes in to the file to begin the read.
        size: How many bytes to read.
        """
        f = self.f

        assert size >= 0, "Can't read a negative amount"
        if size == 0:
            return b""

        got_data = b""

        while True:
            size_now = size
            if start_byte + size > self.bytes_per_cluster:
                size_now = self.bytes_per_cluster - start_byte

            if start_byte < self.bytes_per_cluster:
                size -= size_now

                cluster_bytes_from_root = (head_cluster - 2) * \
                                          self.bytes_per_cluster
                bytes_from_root = cluster_bytes_from_root + start_byte
                bytes_from_data_start = bytes_from_root + self.root_entries * 32

                f.seek(self.data_start() + bytes_from_data_start)
                line = f.read(size_now)
                got_data += line

                if size == 0:
                    return got_data

            start_byte -= self.bytes_per_cluster

            if start_byte < 0:
                start_byte = 0

            f.seek(self.fat_start + head_cluster * 2)
            assert head_cluster <= MAX_CLUSTER_ID, "Out-of-bounds read"
            head_cluster = read_le_short(f)
            assert head_cluster > 0, "Read free cluster"

        return got_data

    def write_cluster_entry(self, entry):
        """
        Write a cluster entry to the FAT table. Assumes our backing file is already
        seeked to the correct entry in the first FAT table.
        """
        f = self.f
        f.write(struct.pack("<H", entry))
        skip_bytes(f, self.fat_size - 2)
        f.write(struct.pack("<H", entry))
        rewind_bytes(f, self.fat_size)

    def allocate(self, amount):
        """
        Allocate a new cluster chain big enough to hold at least the given amount
        of bytes.
        """
        assert amount > 0, "Must allocate a non-zero amount."

        f = self.f
        f.seek(self.fat_start + 4)

        current = None
        current_size = 0
        free_zones = {}

        pos = 2
        while pos < self.fat_size / 2:
            data = read_le_short(f)

            if data == 0 and current is not None:
                current_size += 1
            elif data == 0:
                current = pos
                current_size = 1
            elif current is not None:
                free_zones[current] = current_size
                current = None

            pos += 1

        if current is not None:
            free_zones[current] = current_size

        free_zones = list(free_zones.items())
        free_zones.sort(key=lambda x: x[1])

        grabbed_zones = []
        grabbed = 0

        while grabbed < amount and len(free_zones) > 0:
            zone = free_zones.pop()
            grabbed += zone[1] * self.bytes_per_cluster
            grabbed_zones.append(zone)

        if grabbed < amount:
            return None

        excess = int((grabbed - amount) / self.bytes_per_cluster)

        grabbed_zones[-1] = (grabbed_zones[-1][0],
                             grabbed_zones[-1][1] - excess)

        out = None
        grabbed_zones.reverse()

        for cluster, size in grabbed_zones:
            entries = list(range(cluster + 1, cluster + size))
            entries.append(out or 0xFFFF)
            out = cluster
            f.seek(self.fat_start + cluster * 2)
            for entry in entries:
                self.write_cluster_entry(entry)

        return out

    def extend_cluster(self, cluster, amount):
        """
        Given a cluster which is the *last* cluster in a chain, extend it to hold
        at least `amount` more bytes.
        """
        if amount == 0:
            return
        f = self.f
        entry_offset = self.fat_start + cluster * 2
        f.seek(entry_offset)
        assert read_le_short(f) == 0xFFFF, "Extending from middle of chain"

        return_cluster = self.allocate(amount)
        f.seek(entry_offset)
        self.write_cluster_entry(return_cluster)
        return return_cluster

    def write_file(self, head_cluster, start_byte, data):
        """
        Write to a given FAT file.

        head_cluster: The first cluster in the file.
        start_byte: How many bytes in to the file to begin the write.
        data: The data to write.
        """
        f = self.f
        last_offset = start_byte + len(data)
        current_offset = 0
        current_cluster = head_cluster

        while current_offset < last_offset:
            # Write everything that falls in the cluster starting at current_offset.
            data_begin = max(0, current_offset - start_byte)
            data_end = min(len(data),
                           current_offset + self.bytes_per_cluster - start_byte)
            if data_end > data_begin:
                cluster_file_offset = (self.data_start() + self.root_entries * 32 +
                                       (current_cluster - 2) * self.bytes_per_cluster)
                f.seek(cluster_file_offset + max(0, start_byte - current_offset))
                f.write(data[data_begin:data_end])

            # Advance to the next cluster in the chain or get a new cluster if needed.
                current_offset += self.bytes_per_cluster
            if last_offset > current_offset:
                f.seek(self.fat_start + current_cluster * 2)
                next_cluster = read_le_short(f)
                if next_cluster > MAX_CLUSTER_ID:
                    next_cluster = self.extend_cluster(current_cluster, len(data))
                current_cluster = next_cluster
                assert current_cluster > 0, "Cannot write free cluster"


def copy_file_to_esp(esp_image_path, efi_file_path, dest_path, output_path=None,
                     create_size=None):
    """
    Copy an EFI file into a FAT16 formatted ESP image at the specified destination.

    Args:
        esp_image_path: Path to the ESP image file (FAT16 formatted)
        efi_file_path: Path to the EFI file to copy
        dest_path: Destination path inside the ESP image (e.g., /EFI/BOOT/BOOTAA64.EFI)
        output_path: Output ESP image path (default: overwrite input)
    """
    import shutil

    # If no output path specified, create a temp copy and work on it
    if output_path is None:
        output_path = esp_image_path

    # Create output directory if it doesn't exist
    output_dir = os.path.dirname(output_path)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)

    if esp_image_path is None:
        if output_path is None:
            raise ValueError("output path is required when creating a new ESP image")
        create_empty_fat16_image(output_path, parse_size(create_size or DEFAULT_ESP_SIZE))
    elif esp_image_path != output_path:
        shutil.copy2(esp_image_path, output_path)

    # Open the FAT image
    fs = fat(output_path)
    try:
        root = fs.root

        # Parse destination path and ensure directories exist
        dest_parts = [p for p in dest_path.split('/') if p]

        current_dir = root
        for i, part in enumerate(dest_parts[:-1]):
            existing = current_dir.find_entry(part)
            if existing and existing.is_directory():
                current_dir = existing.open_directory()
            else:
                current_dir = current_dir.open_subdirectory(part)

        # Copy the EFI file
        dest_filename = dest_parts[-1]
        with open(efi_file_path, 'rb') as efi_file:
            current_dir.new_file(dest_filename, efi_file)
    finally:
        fs.close()

    print(f"Successfully copied {efi_file_path} to {dest_path} in {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description='Copy EFI files into FAT16 formatted ESP image.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s -i esp.img -e BOOTAA64.EFI
  %(prog)s -i esp.img -e BOOTAA64.EFI -o esp_out.img
  %(prog)s -i esp.img -e BOOTAA64.EFI -d /EFI/BOOT/BOOTAA64.EFI
  %(prog)s -e BOOTAA64.EFI -o esp.img --size 16M
        """
    )

    parser.add_argument('-i', '--input',
                        help='Input ESP image file (FAT16 formatted)')
    parser.add_argument('-e', '--efi', required=True,
                        help='EFI file to copy')
    parser.add_argument('-o', '--output',
                        help='Output ESP image file (default: overwrite input)')
    parser.add_argument('-s', '--size', default=DEFAULT_ESP_SIZE,
                        help='Create a new FAT16 ESP image of the given size if --input is omitted '
                             f'(default: {DEFAULT_ESP_SIZE})')
    parser.add_argument('-d', '--dest', default='/EFI/BOOT/BOOTAA64.EFI',
                        help='Destination path in ESP image (default: /EFI/BOOT/BOOTAA64.EFI)')

    args = parser.parse_args()

    if args.input and not os.path.exists(args.input):
        print(f"Error: Input ESP image not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    if not os.path.exists(args.efi):
        print(f"Error: EFI file not found: {args.efi}", file=sys.stderr)
        sys.exit(1)

    if not args.input and not args.output:
        print("Error: --output is required when --input is omitted", file=sys.stderr)
        sys.exit(1)

    try:
        copy_file_to_esp(args.input, args.efi, args.dest, args.output, args.size)
    except ValueError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
