"""Generate the production field schema and a separate test-only manual oracle table."""
import argparse
import json
import re
from pathlib import Path

NATIVE_ROOT = Path(__file__).resolve().parents[3]
TESTDATA = NATIVE_ROOT / 'games' / 'zzz' / 'testdata'
INCLUDE = NATIVE_ROOT / 'games' / 'zzz' / 'include'
FIELDS = (
    'touch_count_slot', 'get_touch_slot', 'touch_supported_slot', 'frame_count_slot',
    'screen_width_slot', 'screen_height_slot', 'ui_class_slot', 'static_reference_pool',
    'ui_state_offset', 'get_layout_override', 'set_layout_override', 'get_effective_layout',
    'class_initialized_offset', 'override_property_offset', 'default_property_offset',
)

def profiles():
    rows = [json.loads(path.read_text(encoding='utf-8')) for path in sorted(TESTDATA.glob('[0-9]*.json')) if path.stem in ('3.1', '3.2')]
    assert rows and len({row['sha256'] for row in rows}) == len(rows), 'Missing or duplicate profiles'
    for row in rows:
        assert re.fullmatch(r'[0-9a-f]{64}', row['sha256'])
        assert re.fullmatch(r'[0-9.]+', row['version'])
        assert set(row['values']) == set(FIELDS)
        assert all(0 < int(row['values'][field], 0) < 2**32 for field in FIELDS)
    return rows

def generated():
    lines = ['// Generated field schema; no client addresses or fingerprints.',
             '#pragma once', '#include <cstdint>',
             'namespace profile {', 'struct Build {']
    lines += [f'    uintptr_t {field};' for field in FIELDS]
    lines += ['};', 'struct Field { const char* name; uintptr_t Build::* value; };', 'inline constexpr Field fields[] = {']
    lines += [f'    {{"{field}", &Build::{field}}},' for field in FIELDS]
    lines += ['};', '}', '']
    return '\n'.join(lines)

def test_generated():
    rows = [json.loads((TESTDATA/f'{v}.json').read_text(encoding='utf-8')) for v in ('2.5', '2.6')] + profiles()
    lines = ['// Generated manual oracles. Include in tests only.', '#pragma once',
             '#include "profile.hpp"', '#include <string_view>', 'namespace test_profile {',
             'struct Oracle { const char* version; const char* sha256; profile::Build build;']
    lines += ['};', 'inline constexpr Oracle oracles[] = {']
    for row in rows:
        lines += ['    {', f'        "{row["version"]}", "{row["sha256"]}", {{']
        lines += [f'        {int(row["values"][field], 0):#x}, // {field}' for field in FIELDS]
        lines += ['    }},']
    lines += ['};', '}', '']
    return '\n'.join(lines)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    for name, text in [('profile.hpp', generated()), ('test_profiles.hpp', test_generated())]:
        path = INCLUDE/name
        if args.check:
            if path.read_text(encoding='utf-8') != text:
                raise SystemExit(f'Generated {name} is stale; run scripts/generate_touch_profiles.py')
        else:
            path.write_text(text, encoding='utf-8')
    print('PASS: field schema and separate test oracles match sources')

if __name__ == '__main__':
    main()
