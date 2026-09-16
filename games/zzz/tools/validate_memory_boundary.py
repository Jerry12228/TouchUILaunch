"""Check the production memory resolver's dependency boundary and built DLL imports."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

NATIVE_ROOT = Path(__file__).resolve().parents[3]
ZZZ_ROOT = NATIVE_ROOT / 'games' / 'zzz'
TESTDATA = ZZZ_ROOT / 'testdata'
INCLUDE_ROOTS = (
    NATIVE_ROOT,
    NATIVE_ROOT / 'common' / 'windows' / 'include',
    NATIVE_ROOT / 'common' / 'binary' / 'include',
    NATIVE_ROOT / 'common' / 'process' / 'include',
    NATIVE_ROOT / 'launcher' / 'include',
    NATIVE_ROOT / 'games' / 'gi' / 'include',
    NATIVE_ROOT / 'games' / 'sr' / 'include',
    NATIVE_ROOT / 'games' / 'ww' / 'include',
    ZZZ_ROOT / 'include',
)


def closure(entry):
    result = {}
    def visit(path):
        if path in result:
            return
        source = path.read_text(encoding='utf-8')
        result[path] = source
        # Test-only harnesses are compiled in a separate object target and are
        # absent from the release DLL dependency closure.
        production_source = re.sub(r'#if defined\(TOUCHUI_BRIDGE_TESTING\).*?#endif', '', source, flags=re.S)
        for name in re.findall(r'^#include "([^"]+)"', production_source, re.M):
            candidates = (path.parent / name, *(root / name for root in INCLUDE_ROOTS))
            dependency = next((candidate for candidate in candidates if candidate.is_file()), None)
            assert dependency, f'Cannot resolve private include {name} from {path}'
            visit(dependency)
    visit(entry)
    return result


def imported_dlls(path):
    raw = path.read_bytes()
    u16 = lambda offset: struct.unpack_from('<H', raw, offset)[0]
    u32 = lambda offset: struct.unpack_from('<I', raw, offset)[0]
    pe = u32(0x3c)
    assert u32(pe) == 0x4550 and u16(pe+24) == 0x20b
    table = pe+24+u16(pe+20)
    sections = [struct.unpack_from('<IIII', raw, table+40*i+8) for i in range(u16(pe+6))]
    def offset(rva):
        for virtual_size, address, raw_size, pointer in sections:
            if address <= rva < address+raw_size:
                return pointer+rva-address
        raise ValueError(f'Import RVA outside file: {rva:#x}')
    imports = u32(pe+24+112+8)
    names = []
    if imports:
        position = offset(imports)
        while any(raw[position:position+20]):
            name = offset(u32(position+12))
            names.append(raw[name:raw.index(b'\0', name)].decode('ascii'))
            position += 20
    return names


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=NATIVE_ROOT/'dist')
    args = parser.parse_args()
    runtime = closure(ZZZ_ROOT/'include/profile_resolver.hpp')
    forbidden = r'\b(?:CreateFile\w*|ReadFile|MapViewOfFile|CreateFileMapping\w*|GetModuleFileName\w*|BCrypt\w*|file_sha256|sha256|probe_file|MappedFile|LoadLibrary\w*|test_profile)\b'
    for path, source in runtime.items():
        # Comments describe the boundary; only inspect code tokens.
        code = re.sub(r'//[^\n]*|/\*.*?\*/', '', source, flags=re.S)
        assert not re.search(forbidden, code), f'File/hash/test dependency in {path}'
        assert 'filesystem' not in code and 'fstream' not in code
    payload = closure(ZZZ_ROOT/'src/payload.cpp')
    launcher = closure(NATIVE_ROOT/'launcher/src/launcher.cpp')
    for sources in (runtime, payload, launcher):
        assert not any(p.name == 'test_profiles.hpp' for p in sources)
        assert not any('profile::find' in s or 'profile::builds' in s for s in sources.values())
    payload_text = (ZZZ_ROOT/'src/payload.cpp').read_text(encoding='utf-8')
    assert payload_text.count('discovery::resolve_module(') == 1
    assert not any(p.name == 'file_probe.hpp' for p in payload)
    launch = (NATIVE_ROOT/'launcher/src/launcher.cpp').read_text(encoding='utf-8')
    probe_start = launch.index('if(action==L"probe"||action==L"probe-auto")')
    probe_end = launch.index('// Elevate the launcher', probe_start)
    # The sole call (in addition to its definition) is within the read-only branch.
    outside_probe = launch[:probe_start]+launch[probe_end:]
    assert outside_probe.count('probe_assembly(') == 1
    assert launch[probe_start:probe_end].count('probe_assembly(') == 1
    assert launch.count('discovery::probe_file(') == 1
    dll = args.output_dir/'TouchUILaunch.dll'
    imports = imported_dlls(dll)
    assert not any('bcrypt' in name.lower() or 'crypt32' in name.lower() for name in imports)
    oracles = list(TESTDATA.glob('[0-9]*.json'))
    for binary in ('TouchUILaunch.exe', 'TouchUILaunch.dll'):
        raw = (args.output_dir/binary).read_bytes()
        for path in oracles:
            fingerprint = json.loads(path.read_text(encoding='utf-8'))['sha256']
            assert fingerprint.encode() not in raw and fingerprint.encode('utf-16-le') not in raw
    report = {
        'scope': 'Source dependency and binary import checks, not game execution or OS file-I/O tracing.',
        'checks': ['runtime closure contains no file/hash/path/loader API', 'test oracles absent from release include closures',
                   'one payload resolution call', 'launcher probe calls confined to read-only branch',
                   'payload has no diagnostic crypto imports', 'manual SHA-256 strings absent from both release binaries'],
        'runtime_source_digest_encoding': 'UTF-8, normalized LF line endings',
        'runtime_sources': {str(p.relative_to(NATIVE_ROOT)): hashlib.sha256(source.encode('utf-8')).hexdigest() for p, source in runtime.items()},
        'payload_imported_dlls': imports,
        'status': 'passed',
    }
    (args.output_dir/'memory-boundary.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8', newline='\n')
    print('PASS: memory resolver dependency boundary, single runtime call, diagnostic-only file probe, no embedded manual fingerprints')


if __name__ == '__main__':
    main()
