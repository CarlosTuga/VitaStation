from pathlib import Path
import tempfile,subprocess,sys,struct
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temp:
 t=Path(temp);art=t/'assets/livearea/contents';art.mkdir(parents=True)
 for name,w,h in [('icon.png',128,128),('unusual-background-name.png',840,500)]:
  (art/name).write_bytes(b'\x89PNG\r\n\x1a\n'+b'\0\0\0\rIHDR'+struct.pack('>II',w,h))
 for test in ['game_ui','background']:
  sources=[str(p) for p in (r/'src').glob('*.c') if p.name!='main.c' and (test!='background' or p.name!='library.c')]
  if test=='game_ui':sources=[p for p in sources if Path(p).name not in ['bootstrap.c','launcher.c']]
  exe=t/(test+'.exe');subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-Wall','-Wextra','-Werror','-I',str(r/'include')]+sources+[str(r/'tests'/('test_'+test+'.c')),'-lm','-o',str(exe)],check=True)
  subprocess.run([str(exe)],cwd=t,check=True)
