"""Owned synthetic fixtures; no game binaries or IDA are needed."""

from pathlib import Path
import sqlite3
import struct
import tempfile
import unittest

from query_metadata71 import query
from recover_metadata71 import FormatError, PE, bounded, decode_string, recover


def fixture_pe():
    data = bytearray(0x300)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x80)
    data[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<HH", data, 0x84, 0x8664, 1)
    struct.pack_into("<H", data, 0x94, 0xF0)
    struct.pack_into("<H", data, 0x98, 0x20B)
    struct.pack_into("<Q", data, 0xB0, 0x140000000)
    struct.pack_into("<I", data, 0xD0, 0x3000)
    data[0x188:0x190] = b".text\0\0\0"
    struct.pack_into("<4I", data, 0x190, 0x200, 0x1000, 0x100, 0x200)
    struct.pack_into("<I", data, 0x1AC, 0x60000020)
    data[0x220:0x224] = b"test"
    return data


class RecoveryTests(unittest.TestCase):
    def test_ranges_reject_negative_and_overflow(self):
        self.assertEqual(bounded(b"abc", 3, 0), b"")
        for offset, size in ((-1, 1), (0, -1), (3, 1), (1 << 64, 0)):
            with self.subTest(offset=offset, size=size), self.assertRaises(FormatError):
                bounded(b"abc", offset, size)

    def test_pe_rva_is_not_file_offset(self):
        pe = PE(bytes(fixture_pe()))
        self.assertEqual(pe.read(0x1020, 4), b"test")
        self.assertTrue(pe.executable(0x1100))
        self.assertFalse(pe.executable(0x2000))
        # Virtual memory is larger than raw data; never substitute zeroes.
        for rva, size in ((0x220, 4), (0x1100, 1), (0x10FE, 4)):
            with self.subTest(rva=rva), self.assertRaises(FormatError):
                pe.read(rva, size)

    def test_truncated_and_wrong_architecture_pe(self):
        for data in (b"", bytes(fixture_pe())[:0x100], bytes(fixture_pe())[:0x250]):
            with self.assertRaises(FormatError):
                PE(data)
        data = fixture_pe()
        struct.pack_into("<H", data, 0x84, 0x14C)
        with self.assertRaisesRegex(FormatError, "x64"):
            PE(bytes(data))

    def test_short_string_and_sentinels(self):
        cipher = bytes.fromhex("810e96d358f5cb22")
        self.assertEqual(decode_string(cipher, 0, 0x08000000), "metadata")
        self.assertEqual(decode_string(b"", 0, 0xFFFFFFFF), "")
        self.assertEqual(decode_string(b"", 0, 0), "")

    def test_multiblock_unicode_and_nonzero_pool_offset(self):
        cipher = bytes(37) + bytes.fromhex(
            "ee3a8b03628f6c7a5cd4ded8ba5d430031392875ff065bbe1b0c603f38902d2b"
            "52b897e07b90258f330b46bcaf453269ca8f34cc22df84134822ec5138dd09d0"
            "c40804c800951634edae7a77c407b6e61d40af3cc616fb5411f95cf00fc4ff9c")
        self.assertEqual(decode_string(b"prefix" + cipher, 6, 0x5F000025),
                         "boundary-crossing string with more than 64 bytes / 中文 / end plus another 32 characters here")

    def test_bad_strings_are_rejected(self):
        cipher = bytes.fromhex("810e96d358f5cb22")
        with self.assertRaises(FormatError):
            decode_string(cipher[:7], 0, 0x08000000)
        with self.assertRaisesRegex(FormatError, "UTF-8"):
            decode_string(bytes([cipher[0] ^ 0x92]) + cipher[1:], 0, 0x08000000)
        with self.assertRaisesRegex(FormatError, "NUL"):
            decode_string(bytes([cipher[0] ^ ord("m")]) + cipher[1:], 0, 0x08000000)

    def test_unknown_input_creates_no_artifacts(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            exe = root / "wrong.exe"
            exe.write_bytes(fixture_pe())
            with self.assertRaisesRegex(FormatError, "Unsupported exe SHA-256"):
                recover(exe, root / "absent.dat", root / "absent-startup.dat", root / "out")
            self.assertFalse((root / "out").exists())

    def test_readonly_query_uses_literal_matches(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "metadata.sqlite"
            with sqlite3.connect(path) as db:
                db.execute("CREATE TABLE types(id INTEGER, namespace TEXT, name TEXT)")
                db.executemany("INSERT INTO types VALUES (?, ?, ?)",
                               [(1, "Example", "Input"), (2, "", "x%_' OR 1=1 --")])
            db.close()
            self.assertEqual(query(path, "types", "input", None, 10)[0]["id"], 1)
            self.assertEqual(query(path, "types", "%_' OR 1=1 --", None, 10)[0]["id"], 2)
            self.assertEqual(query(path, "types", "", 1, 10)[0]["name"], "Input")
            with self.assertRaises(ValueError):
                query(path, "types", "", None, 0)
            with self.assertRaises(sqlite3.OperationalError):
                query(Path(folder) / "missing.sqlite", "types", "", None, 10)
            self.assertFalse((Path(folder) / "missing.sqlite").exists())


if __name__ == "__main__":
    unittest.main()
