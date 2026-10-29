from pathlib import Path
import sys,tempfile,subprocess
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
 obj=Path(tmp)/'json.o';exe=Path(tmp)/'snapshot.exe'
 subprocess.run([sys.executable,'-m','ziglang','cc','-c',str(r/'src/json_parser.c'),'-o',str(obj)],check=True)
 subprocess.run([sys.executable,'-m','ziglang','c++','-std=c++17','-DFMT_HEADER_ONLY','-Wall','-Wextra','-Wno-misleading-indentation','-I',str(r/'include'),'-I',str(r/'third_party'),str(r/'src/safety_backup.cpp'),str(r/'third_party/usagi/sha256.cpp'),str(r/'tests/test_safety_backup.cpp'),str(obj),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],cwd=tmp,check=True)
