from pathlib import Path
import tempfile, subprocess, sys
r = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as t:
    stub = Path(t)
    for name, decl in [('appmgr.h', 'int sceAppMgrLaunchAppByUri(int,const char*);'),
                       ('io/fcntl.h', 'int sceIoRemove(const char*);')]:
        p = stub / 'psp2' / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(decl + '\n')
    exe = stub / 'native.exe'
    subprocess.run([sys.executable, '-m', 'ziglang', 'cc', '-std=c99',
                    '-Wall', '-Wextra', '-Werror', '-D__vita__',
                    '-I', str(stub), '-I', str(r / 'include'),
                    str(r / 'tests/test_native_launch.c'), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], cwd=t, check=True)
