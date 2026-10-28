from pathlib import Path
import tempfile,sys,subprocess
p=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temp:
 t=Path(temp);(t/'psp2').mkdir();(t/'psp2/appmgr.h').write_text('');exe=t/'test.exe';subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-Wall','-Wextra','-Werror','-I',str(t),'-I',str(p/'include'),str(p/'tests/test_launch_handoff.c'),'-o',str(exe)],check=True);subprocess.run([str(exe)],cwd=t,check=True)
