from pathlib import Path
import subprocess,sys,tempfile
p=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='vitastation-pkg-tests-') as tmp:
    exe=Path(tmp)/'pkg-engine.exe'
    subprocess.run([sys.executable,'-m','ziglang','c++','-std=c++17','-DFMT_HEADER_ONLY','-Wall','-Wextra','-Werror','-Wno-nullability-completeness','-I',str(p/'third_party'),str(p/'tests/test_pkg_engine.cpp'),str(p/'third_party/usagi/aes128.cpp'),str(p/'third_party/usagi/sha256.cpp'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],cwd=tmp,check=True)
