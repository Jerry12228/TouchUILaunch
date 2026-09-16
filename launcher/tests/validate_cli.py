"""Exercise CLI output and argument rejection without UAC or game operations."""
import argparse
import subprocess
import shutil
import uuid
import time
import ctypes
from contextlib import contextmanager
from pathlib import Path


@contextmanager
def fixture_directory(parent):
    # mkdir's inherited ACL works with restricted Windows tokens, unlike
    # Python 3.13 TemporaryDirectory's private mode-0700 DACL.
    fixture = parent / ('cli fixture 中文 ' + uuid.uuid4().hex)
    fixture.mkdir()
    try:
        yield fixture
    finally:
        assert fixture.resolve().parent == parent.resolve() and not fixture.is_symlink()
        shutil.rmtree(fixture)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('launcher', type=Path)
    parser.add_argument('--probe-game', type=Path)
    args = parser.parse_args()
    launcher = args.launcher.resolve()
    checks = 0

    def run(options, code, logged=False, help_output=False):
        nonlocal checks
        result = subprocess.run([str(launcher), *options], capture_output=True, timeout=120)
        assert result.returncode == code, (options, result.returncode, result.stderr)
        if logged or help_output:
            assert result.stdout or result.stderr, (options, 'expected output')
        else:
            assert not result.stdout and not result.stderr, (options, 'expected silence')
        checks += 1
        return result

    for help_flag in ('--help', '-h'):
        result = run([help_flag], 0, help_output=True)
        assert b'TouchUILaunch' in result.stdout
        assert b'--log' in result.stdout
        assert all(flag in result.stdout for flag in (b'--GI', b'--SR', b'--ZZZ', b'--WW'))
        assert b'-CloudGame -CloudGamePlatform=Android' in result.stdout
        for removed in (b'--status', b'--disable', b'--enable', b'--pid'):
            assert removed not in result.stdout

    for options in (['--status'], ['--disable'], ['--enable'], ['--pid', '12345'],
                    ['--unknown'], ['--game'], ['--ZZZ', '--probe', '--probe'], [],
                    ['--GI'], ['--SR'], ['--GI', '--SR'], ['--ZZZ', '--GI'],
                    ['--GI', '--GI'], ['--SR', '--SR'], ['--ZZZ', '--ZZZ'],
                    ['--GI', '--probe'], ['--SR', '--probe-auto'],
                    ['--WW'], ['--WW', '--WW'], ['--WW', '--GI'],
                    ['--SR', '--WW'], ['--ZZZ', '--WW'],
                    ['--WW', '--probe'], ['--WW', '--probe-auto'],
                    ['--ZZZ', '--game', 'a', '--game', 'b']):
        run(options, 2)
        result = run(['--log', *options], 2, logged=True)
        assert b'ERROR:' in result.stderr
        # Diagnostics work even if an invalid option occurs before --log.
        if options != ['--game']:
            run([*options, '--log'], 2, logged=True)

    missing_game = str(launcher.parent / 'missing-cli-fixture' / 'ZenlessZoneZero.exe')
    run(['--ZZZ', '--probe', '--game', missing_game], 2)
    run(['--ZZZ', '--probe', '--game', missing_game, '--log'], 2, logged=True)
    for selection, name in (('--GI', 'YuanShen.exe'), ('--SR', 'StarRail.exe'), ('--ZZZ', 'ZenlessZoneZero.exe'),
                            ('--WW', 'Client-Win64-Shipping.exe')):
        missing = str(launcher.parent / 'missing-cli-fixture' / name)
        run([selection, '--game', missing, '--log'], 2, logged=True)

    # Suspended copies of our tiny fixture, never game binaries. An internal
    # relaunch marker prevents UAC even if the already-running check regresses.
    child_fixture = launcher.parent / 'MobileUITestChild.exe'
    assert child_fixture.is_file(), child_fixture
    with fixture_directory(launcher.parent) as fixture:
        for selection, name in (('--GI', 'YuanShen.exe'), ('--GI', 'GenshinImpact.exe'),
                                ('--SR', 'StarRail.exe'), ('--ZZZ', 'ZenlessZoneZero.exe'),
                                ('--WW', 'Client-Win64-Shipping.exe')):
            exe = fixture / name
            shutil.copyfile(child_fixture, exe)
            for probe in ('--probe', '--probe-auto'):
                if selection != '--ZZZ':
                    result = run([selection, '--game', str(exe), probe, '--log'], 2, logged=True)
                    assert b'support --ZZZ only' in result.stderr
            wrong = '--SR' if selection != '--SR' else '--GI'
            result = run(['--game', str(exe), wrong, '--log'], 2, logged=True)
            assert b'does not match' in result.stderr
            with subprocess.Popen([str(exe)], creationflags=0x4 | subprocess.CREATE_NO_WINDOW) as child:
                try:
                    options = ['--game', str(exe), selection, '--elevation-relaunch']
                    run(options, 2)
                    result = run([*options, '--log'], 2, logged=True)
                    assert b'already running' in result.stderr
                    assert child.poll() is None, 'launcher must not terminate an existing process'
                    # A different installation path still cannot bypass the family check.
                    alternate = fixture / 'another installation'
                    alternate.mkdir(exist_ok=True)
                    shutil.copyfile(child_fixture, alternate / name)
                    result = run([selection, '--game', str(alternate / name), '--log', '--elevation-relaunch'], 2, logged=True)
                    assert b'already running' in result.stderr
                finally:
                    child.kill()
                    child.wait(timeout=10)
        # Copy the launcher alone: WW must not need the touch DLL, game modules,
        # or suspended initialization. The relaunch marker prevents real UAC;
        # a standard token must fail its elevation guard before starting WW.
        standalone = fixture / launcher.name
        shutil.copyfile(launcher, standalone)
        ww = fixture / 'Client-Win64-Shipping.exe'
        result = subprocess.run([str(standalone), '--WW', '--game', str(ww), '--elevation-relaunch'],
                                capture_output=True, timeout=15)
        assert not result.stdout and not result.stderr, 'WW defaults to silence'
        report = fixture / 'ww-launch-passed.txt'
        if ctypes.windll.shell32.IsUserAnAdmin():
            assert result.returncode == 0, (result.returncode, result.stderr)
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline:
                if report.is_file() and report.read_text().startswith('PASS:'):
                    break
                time.sleep(0.05)
            else:
                raise AssertionError('WW child did not receive exact arguments/cwd or survive launcher exit')
        else:
            assert result.returncode == 2, 'WW must require administrator privileges'
            result = subprocess.run([str(standalone), '--WW', '--game', str(ww), '--elevation-relaunch', '--log'],
                                    capture_output=True, timeout=15)
            assert result.returncode == 2 and b'Elevation did not grant administrator privileges' in result.stderr
            assert not report.exists(), 'WW must not start after failed elevation'
            checks += 1
        checks += 1
    if args.probe_game:
        for probe in ('--probe', '--probe-auto'):
            options = [probe, '--game', str(args.probe_game.resolve()), '--ZZZ']
            run(options, 0)
            result = run([*options, '--log'], 0, logged=True)
            assert b'Probe passed.' in result.stdout
    print(f'PASS: {checks} CLI checks for {launcher}')


if __name__ == '__main__':
    main()
