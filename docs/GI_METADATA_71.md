# GI 7.1 offline metadata recovery

The independent offline tools in [games/gi/tools](../games/gi/tools) recover a
searchable index from the supplied GI 7.1 EXE, `global-metadata.dat`, and
`startup-metadata.dat`. The schema comes from direct analysis of those files in
IDA. It does not use the existing GI mobile UI resolver, stubs, or reference
implementations. Nothing here changes launcher behavior or executes a game.

## Reproduce

Run from the native repository root with Python 3.10+; no third-party packages
are required. Supply all three files explicitly. Output must be absent or empty.

```powershell
python games/gi/tools/recover_metadata71.py `
  --exe C:/Samples/GI/7.1/GenshinImpact.exe `
  --metadata C:/Samples/GI/7.1/GenshinImpact_Data/Managed/Metadata/global-metadata.dat `
  --startup C:/Samples/GI/7.1/GenshinImpact_Data/Managed/Metadata/startup-metadata.dat `
  --out build/gi71-metadata

python games/gi/tools/query_metadata71.py build/gi71-metadata/metadata.sqlite `
  --kind types --match GlobalVars
python games/gi/tools/query_metadata71.py build/gi71-metadata/metadata.sqlite `
  --kind methods --match GetTouch
python games/gi/tools/query_metadata71.py build/gi71-metadata/metadata.sqlite `
  --kind methods --type-id 2323 --limit 200
```

The tool rejects different input hashes. Addresses and arithmetic constants
are evidence for this recorded sample, not a version-independent production
resolver. Inputs, recovered databases, and decompiled listings stay outside Git.

## File layout and evidence

Preferred EXE image base: `0x140000000`. Addresses below are RVAs.

| Evidence | RVA | Meaning |
| --- | --- | --- |
| Registration setup | `0x2F2910` | Installs code/metadata registration and the embedded header pointers |
| Code registration | `0x22DE370` | Member `+0x88` points to the method pointer array |
| Metadata registration | `0x2870A88` | Member `+0x48` points to 16-byte type descriptors; `u32(+0x54) ^ 0x6622B34E` gives their count |
| Embedded header | `0x27D4BD0` | Actual runtime header, 528 bytes |
| File loader | `0x523120` | Opens/maps the requested metadata file |
| Metadata initialization | `0x534B80` | Sets body base to mapped global metadata `+0x210`; startup metadata uses its own unshifted base |
| String reader | `0x50F280` | Decodes length/offset string tokens |
| Type materialization | `0x51CD50` → `0x19EAE27C` | Uses 70-byte type records |
| Method materialization | `0x514630`, `0x51D330` | Uses 26-byte method records and the method pointer array |
| Method name/return/parameters | `0x528080`, `0x5282B0`, `0x528560` | Independent method field consumers |
| Field materialization | `0x512870` | Uses 8-byte field records |

The file begins with `MHY\0` and a different 528-byte header. Runtime reads
the header embedded in the EXE instead. Treating file-header words as table
offsets produces invalid ranges. The separate trailer-based routine at RVA
`0x8AF790` references the same filename but is not the loader used by the
observed metadata initialization; its trailer marker does not match this file.

Let `H[n]` denote an unsigned 32-bit word at byte offset `n` in the embedded
header. All additions/subtractions of encoded 32-bit fields wrap modulo 2^32.

| Table | Relative offset expression | Size/count | Record size |
| --- | --- | --- | --- |
| Type definitions | `H[180] - 330781793` | `H[224] - 486952835` bytes | 70 |
| Methods | `H[364] - 483030612` | `H[392] ^ 0x1B58E334` bytes | 26 |
| Strings | `H[388] - 1080211659` | Tokens carry individual lengths | Bytes |
| Fields | `H[396] - 336578260` | Type-owned ranges | 8 |
| Parameters | `H[276] ^ 0x3E5D33E6` | Method-owned ranges | 8 |
| Images, in startup metadata | `H[524] - 248195061` | `H[44] - 610644003` bytes | 40 |

Global table offsets add `0x210` for the file offset. Image offsets do not.
For this sample, type definitions start at file offset `0x92350`, methods at
`0x2439414`, strings at `0x444C3F8`, and images at startup offset `0x22C5C`.
The encoded method size has a one-byte remainder after full records. The tool
reports that remainder and does not interpret it as another method.

A string token's high byte is its UTF-8 byte length and low 24 bits are its
offset within the string pool. The `0xFFFFFFFF` sentinel returns an empty string.
The initial 64-bit XOR key is
`0x5C2B4E660E2D0544 * ((0x694418957C890198 * offset) ^ 0x55A357D81EF0E48B)`.
Each little-endian 8-byte block advances the key by `0x6B0B40C349AE61E5`,
with arithmetic modulo 2^64. Record-specific decoding happens before this step;
the implementation documents each formula beside its use.

## Outputs and validation

`report.json` contains provenance, hashes, table spans, counts, checks, and
limitations. `metadata.sqlite` contains `images`, `types`, `methods`, `fields`,
`parameters`, `type_descriptors`, and referenced `strings`. The `method_symbols`
view joins assembly/type/method identities and static EXE RVAs. `images.tsv`
and `types.tsv` support inspection without SQLite. The embedded header is also
saved as `metadata-header.embedded.bin`.

| Recovered entity | Count |
| --- | ---: |
| Images | 75 |
| Named type definitions | 88,902 |
| Additional anonymous trailing type records | 2 |
| Type descriptors in EXE | 683,574 |
| Methods | 733,442 |
| Nonzero static method pointers | 711,021 |
| Fields | 440,172 |
| Parameters | 548,400 |
| Unique referenced string tokens | 633,841 |

Every named type belongs to exactly one image; every method belongs to exactly
one type. Each method's decoded declaring-type field independently agrees with
the owning type's method range. Nonzero method pointers must fall in executable
PE sections. Type indexes and file reads are bounded; names require valid UTF-8.
SQLite primary keys, foreign keys, and integrity checks validate the exported
relationships. The two trailing anonymous type records are preserved with NULL
image ownership and validated sentinel contents, not assigned to an invented
assembly. Type descriptor indexes and type-definition indexes are different
domains: use `type_descriptors.definition_id` where present to join them.

The output is an analysis index, not a standard IL2CPP metadata conversion.
Properties, events, generic contexts, attributes, and field memory offsets are
not fully normalized. A recovered name/RVA is a lead for further code analysis;
it does not establish an ABI, a safe hook, or mobile UI behavior. Existing
obfuscated names remain obfuscated.

## Verification

```powershell
python -m unittest discover -s games/gi/tools -p test_metadata71.py -v
```

Eight owned-fixture tests cover PE/RVA bounds and architecture rejection,
short/multiple-block/Unicode string decoding, malformed string rejection,
unknown-input rejection before output creation, and literal read-only queries.
These tests are standalone and not part of CTest. The full three-file sample
recovery is a separate external-sample check. C++ builds and real-game launch
are not required to validate these offline Python tools.
