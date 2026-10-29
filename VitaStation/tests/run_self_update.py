from pathlib import Path
import tempfile,subprocess,sys,struct
r=Path(__file__).resolve().parents[1]
def sfo(id):
    keys=b'';data=b'';entries=[]
    for key,value in [('TITLE_ID',id),('APP_VER','01.65')]:
        at=len(keys);keys+=key.encode()+b'\0';value=value.encode()+b'\0'
        entries.append(struct.pack('<HHIII',at,0x204,len(value),len(value),len(data)));data+=value
    k=20+16*len(entries)
    return struct.pack('<5I',0x46535000,0x101,k,k+len(keys),len(entries))+b''.join(entries)+keys+data
with tempfile.TemporaryDirectory() as tmp:
    t=Path(tmp)
    for name,id in [('source','VTST00001'),('wrong','OTHER0001')]:
        p=t/name;(p/'sce_sys').mkdir(parents=True);(p/'assets').mkdir()
        (p/'sce_sys/param.sfo').write_bytes(sfo(id));(p/'eboot.bin').write_bytes(b'SCE\0payload')
        (p/'assets/logo.png').write_bytes(b'picture resource')
    obj=t/'json.o';exe=t/'update.exe'
    subprocess.run([sys.executable,'-m','ziglang','cc','-c',str(r/'src/json_parser.c'),'-o',str(obj)],check=True)
    subprocess.run([sys.executable,'-m','ziglang','c++','-std=c++17','-DFMT_HEADER_ONLY','-Wall','-Wextra','-Werror','-I',str(r/'include'),'-I',str(r/'third_party'),str(r/'src/self_update.cpp'),str(r/'third_party/usagi/sha256.cpp'),str(r/'tests/test_self_update.cpp'),str(obj),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],cwd=t,check=True)
    exe=t/'transaction.exe'
    subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-Wall','-Wextra','-Werror','-I',str(r/'include'),str(r/'src/update_transaction.c'),str(r/'tests/test_update_transaction.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],cwd=t,check=True)
