from pathlib import Path
import tempfile,subprocess,sys
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as t:
 obj=Path(t)/'plugins.o';exe=Path(t)/'readiness.exe'
 subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-I',str(r/'include'),'-c',str(r/'src/plugins.c'),'-o',str(obj)],check=True)
 subprocess.run([sys.executable,'-m','ziglang','c++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(r/'include'),str(r/'tests/test_plugins_readiness.cpp'),str(obj),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
