from pathlib import Path
import subprocess,sys,tempfile
p=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='vitastation-alive-files-') as tmp:
 exe=Path(tmp)/'alive-files.exe'
 subprocess.run([sys.executable,'-m','ziglang','c++','-std=c++17','-Wno-nullability-completeness','-I',str(p/'include'),str(p/'tests/test_alive_files.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],cwd=tmp,check=True)
