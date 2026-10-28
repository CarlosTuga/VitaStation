from pathlib import Path
import tempfile,subprocess,sys
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temp:
 for name,sources in [('updater',['src/updater.c','src/json_parser.c','tests/test_updater.c']),('maintenance',['tests/test_maintenance.c'])]:
  exe=Path(temp)/(name+'.exe')
  subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-Wall','-Wextra','-Werror','-I',str(r/'include')]+[str(r/f) for f in sources]+['-o',str(exe)],check=True)
  subprocess.run([str(exe)],cwd=temp,check=True)

with tempfile.TemporaryDirectory() as temp:
 obj=Path(temp)/'pack.o';exe=Path(temp)/'covers.exe'
 subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-DVS_COVER_PACK_ROOT="installed"','-I',str(r/'include'),'-c',str(r/'src/cover_pack.c'),'-o',str(obj)],check=True)
 subprocess.run([sys.executable,'-m','ziglang','c++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(r/'include'),str(r/'tests/test_cover_update.cpp'),str(obj),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],cwd=temp,check=True)
