from pathlib import Path
import subprocess, sys, tempfile
r = Path(__file__).resolve().parents[1]
sources = [str(p) for p in sorted((r/'src').glob('*.c')) if p.name != 'main.c']
with tempfile.TemporaryDirectory() as t:
    exe = Path(t)/'personalization.exe'
    subprocess.run([sys.executable, '-m', 'ziglang', 'cc', '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(r/'include')]+sources+[str(r/'tests/test_personalization.c'), '-o', str(exe), '-lm'], check=True)
    subprocess.run([str(exe)], cwd=t, check=True)
