"""Read-only PE checks for every supported native touch profile; never loads the game."""
import argparse
import json
import struct
from pathlib import Path
from pe import PE, require
from generate_profiles import INCLUDE, NATIVE_ROOT, TESTDATA, generated, profiles


def validate(row, game):
    p = PE(game / 'GameAssembly.dll')
    require(p.sha256 == row['sha256'], f"{row['version']}: GameAssembly SHA-256 mismatch")
    values = {name: int(value, 0) for name, value in row['values'].items()}
    checks = []

    def rip(va, prefix, target):
        require(p.read(va, len(prefix)) == prefix, f'{va:#x}: instruction encoding')
        actual = va + len(prefix) + 4 + struct.unpack('<i', p.read(va+len(prefix), 4))[0]
        require(actual == target, f'{va:#x}: relative target {actual:#x} != {target:#x}')

    for name, address in row['wrappers'].items():
        va = int(address, 0)
        prefix = bytes.fromhex('488b05' if name == 'get_touch_slot' else '48ff25')
        rip(va, prefix, p.base+values[name])
        if name == 'get_touch_slot':
            require(p.read(va+7, 3) == bytes.fromhex('48ffe0'), 'GetTouch injected tailcall ABI')
        require(p.ptr(p.base+values[name]) == 0, f'{name}: zero-initialized PE storage')
        record = {'name': name, 'wrapper_va': hex(va), 'slot_rva': hex(values[name]), 'initial_value': 0}
        evidence = TESTDATA/'ida'/row['version']/f'{va:#x}.asm'
        # Some original 3.1 tiny thunks only have C exports. All six 2.5
        # wrappers now have completed IDA functions; require that new evidence.
        require(row['version'] != '2.5' or evidence.is_file(), 'Missing completed 2.5 IDA wrapper')
        if evidence.is_file():
            info = json.loads(evidence.with_suffix('.json').read_text(encoding='utf-8'))
            require(info['input_sha256'] == p.sha256, 'IDA wrapper sample differs')
            total = 0
            for line in evidence.read_text(encoding='utf-8').splitlines():
                address, code, *_ = line.split()
                expected = bytes.fromhex(code)
                require(p.read(int(address, 0), len(expected)) == expected, 'IDA wrapper bytes differ')
                total += len(expected)
            require(total >= 7, 'Incomplete IDA wrapper')
            record['ida_instruction_bytes_verified'] = total
        checks.append(record)

    # These instruction positions were separately inspected in both IDA instances.
    # They bind readiness fields to the fields actually used by the native getters/setter.
    for name, va, delta, property_name, property_opcode in (
        ('get_layout_override', p.base+values['get_layout_override'], 0, 'override_property_offset', '488bb0'),
        ('set_layout_override', p.base+values['set_layout_override'], 16, 'override_property_offset', '488bb8'),
        ('get_default_layout', int(row['get_default_layout_va'], 0), 0, 'default_property_offset', '488bb0'),
    ):
        positions = row.get('accessor_instructions', {}).get(name)
        if positions:
            class_load, initialized, pool_load, state_load, property_load = (
                int(positions[key], 0) for key in ('class_load', 'initialized', 'pool_load', 'state_load', 'property_load'))
            property_opcode, width = positions['property_opcode'], positions['property_width']
        else:
            class_load, initialized, pool_load, state_load, property_load = (va+offset+delta for offset in (0x12, 0x19, 0x26, 0x2d, 0x3d))
            width = 4
        require(width in (1, 4), 'Unsupported oracle displacement width')
        rip(class_load, bytes.fromhex('488b0d'), p.base+values['ui_class_slot'])
        require(p.read(initialized, 7) == bytes.fromhex('80b9')+struct.pack('<I', values['class_initialized_offset'])+b'\0', 'class initialized offset')
        rip(pool_load, bytes.fromhex('488b05'), p.base+values['static_reference_pool'])
        require(p.read(state_load, 7) == bytes.fromhex('488b80')+struct.pack('<I', values['ui_state_offset']), 'static pool offset')
        expected_field = bytes.fromhex(property_opcode)+values[property_name].to_bytes(width, 'little', signed=True)
        require(p.read(property_load, len(expected_field)) == expected_field, 'provider property offset')
        checks.append({'name': name, 'va': hex(va), 'class_and_pool_references': 'passed', 'property_offset': values[property_name]})

    effective = p.base+values['get_effective_layout']
    rip(effective+0x26, b'\xe8', p.base+values['get_layout_override'])
    rip(effective+0x45, b'\xe9', p.base+values['get_layout_override'])
    rip(effective+0x52, b'\xe9', int(row['get_default_layout_va'], 0))
    checks.append({'name':'get_effective_layout', 'va':hex(effective), 'override_and_default_targets':'passed'})

    # Compare every exported instruction byte for the three injected UI entries,
    # instead of accepting only a short, easily confused getter prologue.
    for name in ('get_layout_override', 'set_layout_override', 'get_effective_layout'):
        va = p.base+values[name]
        evidence = TESTDATA/'ida'/row['version']/f'{va:#x}.asm'
        n = 0
        for line in evidence.read_text(encoding='utf-8').splitlines():
            address, code, *_ = line.split()
            expected = bytes.fromhex(code)
            require(p.read(int(address, 0), len(expected)) == expected, f'{name}: IDA instruction mismatch')
            n += len(expected)
        require(n > 100, f'{name}: incomplete IDA evidence')
        checks.append({'name':name, 'ida_instruction_bytes_verified':n})
    return {'version':row['version'], 'game_assembly_sha256':p.sha256, 'game_dir':str(game), 'checks':checks,
            'scope':'Static profile, instruction and storage validation. Does not prove game injection, hotfix behavior or gameplay.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', help='Only validate this supported version; default: all')
    parser.add_argument('--game-dir', type=Path, help='Alternate input directory (requires --version)')
    parser.add_argument('--output-dir', type=Path, default=NATIVE_ROOT/'dist')
    parser.add_argument('--extra-profile', type=Path, action='append', default=[], help='Additional test-only oracle; does not change runtime profiles')
    args = parser.parse_args()
    require(not args.game_dir or args.version, '--game-dir requires --version')
    require((INCLUDE/'profile.hpp').read_text(encoding='utf-8') == generated(), 'Generated profile.hpp is stale')
    candidates = profiles()+[json.loads(path.read_text(encoding='utf-8')) for path in args.extra_profile]
    rows = [r for r in candidates if not args.version or r['version'] == args.version]
    require(bool(rows), 'Unknown profile version')
    reports = [validate(row, args.game_dir or ROOT/row['default_game_dir']) for row in rows]
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for report in reports:
        path = args.output_dir/f"profile-validation-{report['version']}.json"
        path.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
        print(f"PASS client {report['version']}: 6 intrinsic slots, UI property storage, effective-layout calls and full IDA instruction evidence")
    (args.output_dir/'profile-validation.json').write_text(json.dumps({'profiles':reports}, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
