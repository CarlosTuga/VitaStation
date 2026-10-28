from pathlib import Path
import sys,tempfile,subprocess,shutil,zipfile
p=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='psp-bubble-tests-') as temp:
 t=Path(temp);inc=t/'mocks'
 for name in ['psp2/promoterutil.h','psp2/sysmodule.h','psp2/kernel/processmgr.h','psp2/kernel/threadmgr.h','psp2/kernel/sysmem.h','psp2/rtc.h','psp2/io/stat.h','psp2/io/fcntl.h','zlib.h']:
  f=inc/name;f.parent.mkdir(exist_ok=True,parents=True);f.write_text('')
 shutil.copytree(p/'assets/psp-runtime',t/'fs/app0/assets/psp-runtime');(t/'fs/app0/sce_sys').mkdir();shutil.copy2(p/'assets/icon0.png',t/'fs/app0/sce_sys/icon0.png')
 # Original official v7 modules extracted from upstream RetroFlow's licensed payload.
 archive=Path(sys.argv[1]) if len(sys.argv)>1 else Path('work/psp160/RetroFlow.vpk')
 with zipfile.ZipFile(archive) as z:
  (t/'original').mkdir()
  for name in ['adrenaline_kernel.skprx','adrenaline_user.suprx','adrenaline_vsh.suprx']:(t/'original'/name).write_bytes(z.read('payloads/adrenaline/sce_module/'+name))
 exe=t/'tests.exe';subprocess.run([sys.executable,'-m','ziglang','c++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(inc),'-I',str(p/'include'),str(p/'tests/test_bubbles.cpp'),'-o',str(exe)],check=True);subprocess.run([str(exe)],cwd=t,check=True)
