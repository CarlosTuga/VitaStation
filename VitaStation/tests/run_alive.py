from pathlib import Path
import subprocess,sys,tempfile
p=Path(__file__).resolve().parents[1]
sources=[str(f) for f in sorted((p/'src').glob('*.c')) if f.name!='main.c']
with tempfile.TemporaryDirectory(prefix='vitastation-alive-') as tmp:
 exe=Path(tmp)/'alive.exe'
 subprocess.run([sys.executable,'-m','ziglang','cc','-DVS_STORE_COVERS_DIR="cover-fixture"','-DVS_PACKED_COVERS_DIR="packed-fixture"','-std=c99','-Wall','-Wextra','-Werror','-I',str(p/'include')]+sources+[str(p/'tests/test_alive.c'),'-o',str(exe),'-lm'],check=True)
 subprocess.run([str(exe),str(p/'assets/alive')],cwd=tmp,check=True)
