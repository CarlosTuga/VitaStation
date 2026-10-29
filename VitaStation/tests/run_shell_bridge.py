from pathlib import Path
import tempfile, subprocess, sys
r = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as t:
    stub = Path(t)
    for name in ['appmgr.h', 'ctrl.h', 'io/fcntl.h', 'io/stat.h',
                 'kernel/threadmgr.h', 'kernel/modulemgr.h', 'kernel/clib.h']:
        p = stub / 'psp2' / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('/* SDK simulation declarations are in test_shell_bridge.c. */\n')
    exe = stub / 'bridge.exe'
    subprocess.run([sys.executable, '-m', 'ziglang', 'cc', '-std=c99',
                    '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                    '-I', str(stub), str(r / 'tests/test_shell_bridge.c'),
                    '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
