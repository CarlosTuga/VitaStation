from pathlib import Path
import tempfile,subprocess,sys
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as t:
 exe=Path(t)/'startup.exe'
 subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-Wall','-Wextra','-Werror','-I',str(r/'include'),str(r/'src/bootstrap_state.c'),str(r/'tests/test_bootstrap_exit.c'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],cwd=t,check=True)
