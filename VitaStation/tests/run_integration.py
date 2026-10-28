from pathlib import Path
import subprocess,sys,tempfile,shutil
p=Path(__file__).resolve().parents[1]
sources=[str(f) for f in sorted((p/'src').glob('*.c')) if f.name!='main.c']
with tempfile.TemporaryDirectory(prefix='vitastation-integration-') as tmp:
    exe=Path(tmp)/'integration.exe'
    subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-Wall','-Wextra','-Werror','-I',str(p/'include')]+sources+[str(p/'tests/test_integration.c'),'-o',str(exe),'-lm'],check=True)
    args=[str(exe)]+([str(Path(sys.argv[1]).resolve())] if len(sys.argv)>1 else [])
    subprocess.run(args,cwd=tmp,check=True)
