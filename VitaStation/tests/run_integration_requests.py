from pathlib import Path
import tempfile,subprocess,sys
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as t:
 exe=Path(t)/'integrations.exe'
 subprocess.run([sys.executable,'-m','ziglang','c++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(r/'include'),str(r/'tests/test_integration_requests.cpp'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
