"""Small, read-only PE reader shared by ZZZ development tools.

This intentionally covers only the file-backed PE operations used by the
profile and scan-rule validators.  It has no dependency on the parent
repository's metadata-recovery tooling.
"""

from __future__ import annotations

import hashlib
import struct
from pathlib import Path


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


class PE:
    """Read a 64-bit PE image without loading or executing it."""

    def __init__(self, path: Path) -> None:
        self.path = path
        self.data = path.read_bytes()
        self.sha256 = hashlib.sha256(self.data).hexdigest()
        pe = self.u32(0x3C)
        require(self.data[pe:pe + 4] == b"PE\0\0", "Expected PE image")
        require(self.u16(pe + 4) == 0x8664, "Expected x64 PE image")
        opt = pe + 24
        require(self.u16(opt) == 0x20B, "Expected PE32+ image")
        self.base = self.u64(opt + 24)
        table = opt + self.u16(pe + 20)
        self.sections: list[tuple[int, int, int, bytes]] = []
        self.virtual_sections: list[tuple[int, int, int]] = []
        for index in range(self.u16(pe + 6)):
            offset = table + index * 40
            virtual_size, rva, raw_size, raw = struct.unpack_from("<IIII", self.data, offset + 8)
            self.sections.append((rva, raw_size, raw, self.data[offset:offset + 8].rstrip(b"\0")))
            self.virtual_sections.append((rva, max(virtual_size, raw_size), raw_size))

    def u16(self, offset: int) -> int:
        return struct.unpack_from("<H", self.data, offset)[0]

    def u32(self, offset: int) -> int:
        return struct.unpack_from("<I", self.data, offset)[0]

    def u64(self, offset: int) -> int:
        return struct.unpack_from("<Q", self.data, offset)[0]

    def offset(self, va: int, size: int = 1) -> int:
        rva = va - self.base
        for start, length, raw, _ in self.sections:
            if start <= rva and rva + size <= start + length:
                return raw + rva - start
        raise ValueError(f"VA {va:#x} size {size} is not file-backed")

    def read(self, va: int, size: int) -> bytes:
        offset = self.offset(va, size)
        return self.data[offset:offset + size]

    def uint(self, va: int) -> int:
        return struct.unpack("<I", self.read(va, 4))[0]

    def ptr(self, va: int) -> int:
        try:
            return struct.unpack("<Q", self.read(va, 8))[0]
        except ValueError:
            rva = va - self.base
            if any(start + raw_size <= rva and rva + 8 <= start + virtual_size
                   for start, virtual_size, raw_size in self.virtual_sections):
                return 0
            raise
