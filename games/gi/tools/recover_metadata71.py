"""Recover a searchable GI 7.1 metadata index from explicitly supplied files.

This is an offline research tool, independent of the launcher and its resolver.
The schema was derived from the supplied EXE in IDA; see GI_METADATA_71.md.
Only Python's standard library is required. Game files are never executed.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import sqlite3
import struct
import sys


MASK32 = (1 << 32) - 1
MASK64 = (1 << 64) - 1
HEADER_SIZE = 0x210
HEADER_RVA = 0x27D4BD0
CODE_REGISTRATION_RVA = 0x22DE370
METADATA_REGISTRATION_RVA = 0x2870A88
EXPECTED_HASHES = {
    "exe": "08a3086d5f3fe695f01dab61efa42e442006b18e5e475b2520df356f6a073b7d",
    "metadata": "05ae04d7a91b91cc880217a56b0b01f3e67f845b06e894216654ec5d160e0da0",
    "startup": "66c684950af332966887cfd6f5263a05704230999ee290c27d5b3bdb8a713923",
}


class FormatError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise FormatError(message)


def bounded(data: bytes, offset: int, size: int) -> bytes:
    require(0 <= offset <= len(data) and 0 <= size <= len(data) - offset,
            f"Range outside input: offset={offset:#x}, size={size:#x}")
    return data[offset:offset + size]


def u16(data: bytes, offset: int) -> int:
    return int.from_bytes(bounded(data, offset, 2), "little")


def u32(data: bytes, offset: int) -> int:
    return int.from_bytes(bounded(data, offset, 4), "little")


def u64(data: bytes, offset: int) -> int:
    return int.from_bytes(bounded(data, offset, 8), "little")


def i32(value: int) -> int:
    value &= MASK32
    return value if value < 0x80000000 else value - (1 << 32)


class PE:
    def __init__(self, data: bytes):
        self.data = data
        require(bounded(data, 0, 2) == b"MZ", "Not a DOS/PE image")
        nt = u32(data, 0x3C)
        require(bounded(data, nt, 4) == b"PE\0\0", "Invalid PE signature")
        require(u16(data, nt + 4) == 0x8664, "Expected an x64 PE")
        opt = nt + 24
        require(u16(data, opt) == 0x20B, "Expected PE32+")
        self.image_base = u64(data, opt + 24)
        self.image_size = u32(data, opt + 56)
        self.sections = []
        table = opt + u16(data, nt + 20)
        for n in range(u16(data, nt + 6)):
            at = table + n * 40
            name = bounded(data, at, 8).rstrip(b"\0").decode("ascii", "replace")
            vs, va, raw_size, raw = struct.unpack("<4I", bounded(data, at + 8, 16))
            bounded(data, raw, raw_size)
            self.sections.append((name, va, vs, raw, raw_size, u32(data, at + 36)))

    def file_offset(self, rva: int, size: int) -> int:
        for _, va, _, raw, raw_size, _ in self.sections:
            if va <= rva and rva - va + size <= raw_size:
                return raw + rva - va
        raise FormatError(f"RVA has no file backing: {rva:#x} + {size:#x}")

    def read(self, rva: int, size: int) -> bytes:
        return bounded(self.data, self.file_offset(rva, size), size)

    def executable(self, rva: int) -> bool:
        return any(va <= rva < va + max(vs, raw_size) and flags & 0x20000000
                   for _, va, vs, _, raw_size, flags in self.sections)


def decode_string(data: bytes, base: int, token: int) -> str:
    """0x14050F280: high byte = length, low 24 bits = encrypted pool offset."""
    token &= MASK32
    if token == MASK32:
        return ""
    length, offset = token >> 24, token & 0xFFFFFF
    if length == 0:
        return ""
    cipher = bounded(data, base + offset, (length + 7) & ~7)
    key = (0x5C2B4E660E2D0544 *
           ((0x694418957C890198 * offset) ^ 0x55A357D81EF0E48B)) & MASK64
    plain = bytearray()
    for index in range(0, len(cipher), 8):
        plain.extend((u64(cipher, index) ^ key).to_bytes(8, "little"))
        key = (key + 0x6B0B40C349AE61E5) & MASK64
    try:
        text = plain[:length].decode("utf-8")
    except UnicodeDecodeError as error:
        raise FormatError(f"Invalid UTF-8 at string token {token:#x}") from error
    require("\0" not in text, f"Embedded NUL at string token {token:#x}")
    return text


class Metadata:
    def __init__(self, pe: PE, data: bytes, startup: bytes):
        self.pe, self.data, self.startup = pe, data, startup
        self.header = pe.read(HEADER_RVA, HEADER_SIZE)
        require(data[:8] == b"MHY\0\0\0\0\0", "Invalid MHY file header")
        require(self.header[:8] == data[:8], "Invalid embedded MHY header")
        self.tables = {}
        # Offsets are relative to global-metadata.dat + 0x210, except images.
        self.table("types", self.hsub(180, 330781793), self.hsub(224, 486952835), 70)
        self.table("methods", self.hsub(364, 483030612), self.hxor(392, 0x1B58E334), 26)
        self.strings_base = HEADER_SIZE + self.hsub(388, 1080211659)
        self.fields_base = HEADER_SIZE + self.hsub(396, 336578260)
        self.parameters_base = HEADER_SIZE + self.hxor(276, 0x3E5D33E6)
        self.images_base = self.hsub(524, 248195061)
        image_bytes = self.hsub(44, 610644003)
        require(image_bytes % 40 == 0, "Image table has a partial record")
        self.image_count = image_bytes // 40
        bounded(startup, self.images_base, image_bytes)
        self.tables["images"] = dict(source="startup", offset=self.images_base,
                                     size=image_bytes, stride=40, count=self.image_count, tail=0)
        registration = pe.read(CODE_REGISTRATION_RVA, 144)
        self.pointers_rva = u64(registration, 136) - pe.image_base
        self.pointers = pe.read(self.pointers_rva, self.tables["methods"]["count"] * 8)
        registration = pe.read(METADATA_REGISTRATION_RVA, 96)
        self.descriptor_count = u32(registration, 84) ^ 0x6622B34E
        self.descriptors_rva = u64(registration, 72) - pe.image_base
        self.descriptors = pe.read(self.descriptors_rva, self.descriptor_count * 16)
        self.strings = {}

    def hsub(self, offset: int, constant: int) -> int:
        return (u32(self.header, offset) - constant) & MASK32

    def hxor(self, offset: int, constant: int) -> int:
        return u32(self.header, offset) ^ constant

    def table(self, name: str, relative: int, size: int, stride: int) -> None:
        offset = HEADER_SIZE + relative
        bounded(self.data, offset, size)
        count, tail = divmod(size, stride)
        self.tables[name] = dict(source="metadata", offset=offset, size=size,
                                 stride=stride, count=count, tail=tail)

    def string(self, token: int) -> str:
        token &= MASK32
        if token not in self.strings:
            self.strings[token] = decode_string(self.data, self.strings_base, token)
        return self.strings[token]

    def images(self):
        for index in range(self.image_count):
            p = self.images_base + index * 40
            # Integer widths follow the x64 instructions in 0x140534B80.
            key = (1368137530 * ((667674736 * ((13029 * index) ^ 0x43E2DEB7)
                                 + 0xCB287082F0) >> 8) + 1505552983) & MASK32
            yield dict(id=index, name=self.string(key ^ u32(self.startup, p + 32) ^ 0x772FA51E),
                       type_start=i32(key ^ u32(self.startup, p + 12) ^ 0x38EAB186 ^ 0x4B1CB1C1),
                       type_count=key ^ ((u32(self.startup, p + 20) - 1438863863) & MASK32),
                       file_offset=p)

    def types(self):
        table = self.tables["types"]
        for index in range(table["count"]):
            p = table["offset"] + index * 70
            yield dict(id=index,
                       namespace=self.string(u32(self.data, p) - 1145778368),
                       name=self.string(u32(self.data, p + 36) - 72511848),
                       method_start=i32(u32(self.data, p + 12) ^ 0x4D8127F2),
                       method_count=(u16(self.data, p + 48) + 4806) & 0xFFFF,
                       field_start=i32(u32(self.data, p + 28) ^ 0x29010897),
                       field_count=u16(self.data, p + 56) ^ 0x51A8,
                       flags=(u32(self.data, p + 40) - 1096399758) & MASK32,
                       byval_type=i32(u32(self.data, p + 4) ^ 0x0DA4711B),
                       parent_type=i32(u32(self.data, p + 24) - 1031049247),
                       declaring_type=i32(u32(self.data, p + 16) - 1950851500),
                       file_offset=p)

    def methods(self):
        table = self.tables["methods"]
        for index in range(table["count"]):
            p = table["offset"] + index * 26
            key = (((860405619 * index) ^ 0x73758947) + 1547935323) & MASK32
            va = u64(self.pointers, index * 8)
            rva = va - self.pe.image_base if va else None
            require(rva is None or self.pe.executable(rva),
                    f"Method {index} pointer outside executable sections: {va:#x}")
            yield dict(id=index,
                       name=self.string(((u32(self.data, p) - 1524016681) & MASK32) ^ key),
                       declaring_type=u32(self.data, p + 12) ^ key ^ 0x59244785,
                       return_type=i32(u32(self.data, p + 8) ^ key ^ 0x3F7BDF39),
                       parameter_start=i32(key ^ ((u32(self.data, p + 4) - 257927898) & MASK32)),
                       parameter_count=(key ^ (self.data[p + 24] - 31)) & 0xFF,
                       flags=(key ^ u16(self.data, p + 22) ^ 0x8F60) & 0xFFFF,
                       slot=(key ^ u16(self.data, p + 18) ^ 0x41FB) & 0xFFFF,
                       rva=rva, file_offset=p)

    def type_descriptors(self):
        for index in range(self.descriptor_count):
            p = index * 16
            bits = u32(self.descriptors, p + 8)
            kind = (bits >> 16) & 0xFF
            data = u64(self.descriptors, p)
            definition = i32(data) if kind in (0x11, 0x12) else None
            require(definition is None or 0 <= definition < self.tables["types"]["count"],
                    f"Type descriptor {index} has invalid definition {definition}")
            yield dict(id=index, kind=kind, attrs=bits & 0xFFFF, bits=bits,
                       raw_data=hex(data), definition_id=definition,
                       rva=self.descriptors_rva + p)

    def fields(self, owner: dict):
        for index in range(owner["field_start"], owner["field_start"] + owner["field_count"]):
            p = self.fields_base + index * 8
            key = (((1824013172 * ((34782 * index) ^ 0x59B1DB19)) >> 16) + 1410079245) & MASK32
            yield dict(id=index, declaring_type=owner["id"],
                       name=self.string(((u32(self.data, p + 4) - 1475293452) & MASK32) ^ key ^ 0x2D027B84),
                       type_index=i32(key ^ u32(self.data, p) ^ 0x2D27D873), file_offset=p)

    def parameters(self, owner: dict):
        start = owner["parameter_start"]
        for index in range(start, start + owner["parameter_count"]):
            p = self.parameters_base + index * 8
            key = (((1457992407 * ((41617 * index + 1219887025) ^ 0x19E1D47A)) >> 21)
                   + 2068375556) & MASK32
            yield dict(id=index, method_id=owner["id"], position=index - start,
                       name=self.string(key ^ ((u32(self.data, p + 4) - 1764493660) & MASK32) ^ 0x4CDBD093),
                       type_index=i32(u32(self.data, p) ^ key ^ 0x31BF59F3), file_offset=p)


def insert_rows(db, name, rows):
    iterator = iter(rows)
    first = next(iterator, None)
    if first is None:
        return 0
    keys = list(first)
    sql = f'INSERT INTO {name} ({",".join(keys)}) VALUES ({",".join("?" for _ in keys)})'
    db.execute(sql, list(first.values()))
    count = 1
    for row in iterator:
        db.execute(sql, [row[k] for k in keys])
        count += 1
    return count


def recover(exe_path: Path, metadata_path: Path, startup_path: Path, out: Path) -> dict:
    paths = dict(exe=exe_path, metadata=metadata_path, startup=startup_path)
    inputs, provenance = {}, {}
    for name, path in paths.items():
        data = path.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        require(digest == EXPECTED_HASHES[name],
                f"Unsupported {name} SHA-256 {digest}; this schema is for the recorded GI 7.1 sample")
        inputs[name] = data
        provenance[name] = dict(path=str(path.resolve()), size=len(data), sha256=digest)
    require(not out.exists() or not any(out.iterdir()), "Output directory must be absent or empty")
    pe = PE(inputs["exe"])
    metadata = Metadata(pe, inputs["metadata"], inputs["startup"])
    images = list(metadata.images())
    types = list(metadata.types())
    method_count = metadata.tables["methods"]["count"]
    type_owners = [-1] * len(types)
    method_owners = [-1] * method_count
    for im in images:
        start, count = im["type_start"], im["type_count"]
        require(count == 0 or 0 <= start <= start + count <= len(types), f"Image {im['id']} range invalid")
        for index in range(start, start + count):
            require(type_owners[index] == -1, f"Type {index} belongs to multiple images")
            type_owners[index] = im["id"]
    # Two trailing anonymous sentinel records are outside all image ranges.
    # Preserve them as unowned records; do not invent an assembly for them.
    unowned = [ty for ty in types if type_owners[ty["id"]] == -1]
    require([ty["id"] for ty in unowned] == [len(types) - 2, len(types) - 1]
            and [ty["flags"] for ty in unowned] == [0, 128]
            and all(ty["name"] == ty["namespace"] == "" and ty["byval_type"] == -1
                    and ty["method_start"] == ty["field_start"] == -1
                    and ty["method_count"] == ty["field_count"] == 0 for ty in unowned),
            "Unexpected unowned types; expected the two trailing sentinel records")
    for ty in types:
        ty["image_id"] = type_owners[ty["id"]] if type_owners[ty["id"]] >= 0 else None
        start, count = ty["method_start"], ty["method_count"]
        require(count == 0 or 0 <= start <= start + count <= method_count, f"Type {ty['id']} method range invalid")
        require(ty["field_count"] == 0 or ty["field_start"] >= 0, f"Type {ty['id']} field range invalid")
        for index in range(start, start + count):
            require(method_owners[index] == -1, f"Method {index} belongs to multiple types")
            method_owners[index] = ty["id"]
    require(-1 not in method_owners, "Some methods have no owning type")
    out.mkdir(parents=True, exist_ok=True)
    database = out / "metadata.sqlite"
    # The .partial suffix prevents an interrupted run from looking validated.
    partial = database.with_suffix(".sqlite.partial")
    counts = {"sentinel_types": len(unowned)}
    try:
        with sqlite3.connect(partial) as db:
            db.execute("PRAGMA foreign_keys=ON")
            db.executescript('''
                CREATE TABLE images(id INTEGER PRIMARY KEY, name TEXT NOT NULL, type_start INTEGER,
                    type_count INTEGER, file_offset INTEGER);
                CREATE TABLE types(id INTEGER PRIMARY KEY, namespace TEXT NOT NULL, name TEXT NOT NULL,
                    method_start INTEGER, method_count INTEGER, field_start INTEGER, field_count INTEGER,
                    flags INTEGER, byval_type INTEGER, parent_type INTEGER, declaring_type INTEGER,
                    file_offset INTEGER, image_id INTEGER REFERENCES images(id));
                CREATE TABLE methods(id INTEGER PRIMARY KEY, name TEXT NOT NULL,
                    declaring_type INTEGER REFERENCES types(id), return_type INTEGER, parameter_start INTEGER,
                    parameter_count INTEGER, flags INTEGER, slot INTEGER, rva INTEGER, file_offset INTEGER);
                CREATE TABLE fields(id INTEGER PRIMARY KEY, declaring_type INTEGER REFERENCES types(id),
                    name TEXT NOT NULL, type_index INTEGER, file_offset INTEGER);
                CREATE TABLE parameters(id INTEGER PRIMARY KEY, method_id INTEGER REFERENCES methods(id),
                    position INTEGER, name TEXT NOT NULL, type_index INTEGER, file_offset INTEGER);
                CREATE TABLE strings(token INTEGER PRIMARY KEY, name TEXT NOT NULL);
                CREATE TABLE type_descriptors(id INTEGER PRIMARY KEY, kind INTEGER, attrs INTEGER,
                    bits INTEGER, raw_data TEXT, definition_id INTEGER REFERENCES types(id), rva INTEGER);
                CREATE VIEW method_symbols AS SELECT m.id, i.name AS image, t.namespace,
                    t.name AS type_name, m.name, m.parameter_count, m.return_type, m.rva,
                    m.declaring_type, m.file_offset FROM methods m
                    JOIN types t ON t.id=m.declaring_type JOIN images i ON i.id=t.image_id;
            ''')
            counts["images"] = insert_rows(db, "images", images)
            counts["types"] = insert_rows(db, "types", types)
            counts["type_descriptors"] = insert_rows(db, "type_descriptors", metadata.type_descriptors())
            def validated_methods():
                for row in metadata.methods():
                    require(row["declaring_type"] == method_owners[row["id"]],
                            f"Method {row['id']} disagrees with its owning type")
                    require(row["parameter_count"] == 0 or row["parameter_start"] >= 0,
                            f"Method {row['id']} parameter range invalid")
                    yield row
            counts["methods"] = insert_rows(db, "methods", validated_methods())
            print(f"Recovered {counts['types']} types and {counts['methods']} methods", flush=True)
            counts["fields"] = insert_rows(db, "fields", (r for ty in types for r in metadata.fields(ty)))
            db.row_factory = sqlite3.Row
            counts["parameters"] = insert_rows(db, "parameters", (
                r for method in db.execute("SELECT * FROM methods")
                for r in metadata.parameters(dict(method))))
            db.executemany("INSERT INTO strings VALUES (?, ?)", sorted(metadata.strings.items()))
            counts["referenced_strings"] = len(metadata.strings)
            for table, columns in (("types", ("byval_type", "parent_type", "declaring_type")),
                                   ("methods", ("return_type",)), ("fields", ("type_index",)),
                                   ("parameters", ("type_index",))):
                for column in columns:
                    invalid = db.execute(f"SELECT id FROM {table} WHERE {column} < -1 OR {column} >= ? LIMIT 1",
                                         (metadata.descriptor_count,)).fetchone()
                    require(invalid is None, f"{table}.{column} contains an invalid type index: {invalid}")
            db.executescript('''
                CREATE INDEX type_names ON types(name);
                CREATE INDEX method_names ON methods(name);
                CREATE INDEX method_types ON methods(declaring_type);
                CREATE INDEX method_addresses ON methods(rva);
                CREATE INDEX field_names ON fields(name);
                CREATE INDEX field_types ON fields(declaring_type);
            ''')
            require(not list(db.execute("PRAGMA foreign_key_check")), "Broken relational reference")
            require(db.execute("PRAGMA integrity_check").fetchone()[0] == "ok", "SQLite integrity failure")
            for name in ("images", "types"):
                cursor = db.execute(f"SELECT * FROM {name} ORDER BY id")
                with (out / f"{name}.tsv").open("w", encoding="utf-8", newline="") as stream:
                    writer = csv.writer(stream, delimiter="\t")
                    writer.writerow([col[0] for col in cursor.description])
                    writer.writerows(cursor)
            counts["nonzero_method_pointers"] = db.execute("SELECT count(*) FROM methods WHERE rva IS NOT NULL").fetchone()[0]
        # A sqlite3 connection context manager commits but does not close.
        db.close()
        partial.replace(database)
    except BaseException:
        if "db" in locals():
            db.close()
        raise
    (out / "metadata-header.embedded.bin").write_bytes(metadata.header)
    report = dict(schema="gi-7.1-offline-v1", inputs=provenance,
                  image_base=hex(pe.image_base), header_rva=hex(HEADER_RVA),
                  code_registration_rva=hex(CODE_REGISTRATION_RVA),
                  metadata_registration_rva=hex(METADATA_REGISTRATION_RVA),
                  method_pointers_rva=hex(metadata.pointers_rva), tables=metadata.tables,
                  type_descriptors_rva=hex(metadata.descriptors_rva),
                  strings_base=metadata.strings_base, fields_base=metadata.fields_base,
                  parameters_base=metadata.parameters_base, counts=counts,
                  checks=["Exact input hashes", "PE file-backed ranges", "Strict UTF-8 names",
                          "All named types owned once by images; two exact trailing sentinel records",
                          "All methods owned once by types",
                          "Method declaring types agree with type ranges",
                          "Return, parent, declaring, field and parameter type indexes are in range",
                          "Nonzero method pointers in executable sections", "SQLite integrity and foreign keys"],
                  limitations=["Not a standard IL2CPP global-metadata.dat conversion",
                               "No runtime or mobile UI compatibility claim",
                               "Properties, events, generics and attribute tables not normalized",
                               "Method pointers are static EXE evidence; shared and null entries are preserved"])
    (out / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--startup", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    try:
        report = recover(args.exe, args.metadata, args.startup, args.out)
    except (OSError, FormatError, sqlite3.Error) as error:
        print(f"Metadata recovery failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(report["counts"], indent=2))
    print(f"Validated index: {args.out / 'metadata.sqlite'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
