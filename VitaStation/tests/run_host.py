"""Usage: python tests/run_host.py [--zig]. Requires a host C compiler."""
from pathlib import Path
import subprocess,sys,tempfile,shutil
root=Path(__file__).resolve().parents[1]
compiler=[sys.executable,'-m','ziglang','cc'] if '--zig' in sys.argv else [shutil.which('cc') or shutil.which('gcc') or 'clang']
with tempfile.TemporaryDirectory(prefix='vitastation-tests-') as tmp:
    binary=Path(tmp)/('tests.exe' if sys.platform=='win32' else 'tests')
    sources=[str(x) for x in sorted((root/'src').glob('*.c')) if x.name!='main.c']
    subprocess.run(compiler+['-std=c99','-Wall','-Wextra','-Werror','-I',str(root/'include')]+sources+[str(root/'tests/test_vitastation.c'),'-o',str(binary),'-lm'],check=True)
    subprocess.run([str(binary)],cwd=tmp,check=True)
