from pathlib import Path
import subprocess,sys,tempfile
p=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='cover-installer-test-') as tmp:
 exe=Path(tmp)/'test.exe'
 subprocess.run([sys.executable,'-m','ziglang','c++','-std=c++17','-Wno-nullability-completeness',str(p/'tests/test_installer.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],cwd=tmp,check=True)
