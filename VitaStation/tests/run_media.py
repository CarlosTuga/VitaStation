from pathlib import Path
import tempfile,subprocess,sys,struct
root=Path(__file__).resolve().parents[1]
def sfo(title,cat,id):
 fields=[('TITLE',title),('CATEGORY',cat),('DISC_ID',id)];keys=b'';data=b'';entries=[]
 for name,value in fields:
  ko=len(keys);keys+=name.encode()+b'\0';b=value.encode()+b'\0';entries.append((ko,0x204,len(b),len(b),len(data)));data+=b
 k=20+16*len(fields);d=k+len(keys);return struct.pack('<5I',0x46535000,0x101,k,d,len(fields))+b''.join(struct.pack('<HHIII',*e) for e in entries)+keys+data
def record(name,sector,size):
 n=33+len(name);n+=n%2;b=bytearray(n);b[0]=n;struct.pack_into('<I',b,2,sector);struct.pack_into('<I',b,10,size);b[32]=len(name);b[33:33+len(name)]=name;return b
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp)
 for folder,title,cat,id in [('PS1','Test PS1','ME','SLUS00980'),('PSP','Test PSP','MG','ULES01347')]:
  p=t/'games'/folder;p.mkdir(parents=True);b=sfo(title,cat,id);(p/'EBOOT.PBP').write_bytes(struct.pack('<10I',0x50425000,0x10000,40,*([40+len(b)]*7))+b)
 p=t/'games/ISO';p.mkdir();b=bytearray(22*2048);b[16*2048]=1;b[16*2048+1:16*2048+6]=b'CD001';b[16*2048+156:16*2048+190]=record(b'\0',18,2048);r=record(b'PSP_GAME',19,2048);b[18*2048:18*2048+len(r)]=r;m=sfo('Test ISO','UG','ULES00151');r=record(b'PARAM.SFO;1',20,len(m));b[19*2048:19*2048+len(r)]=r;b[20*2048:20*2048+len(m)]=m;(p/'Test.iso').write_bytes(b);(t/'games/bad.pbp').write_bytes(b'bad')
 sources=[str(f) for f in sorted((root/'src').glob('*.c')) if f.name!='main.c'];exe=t/'media.exe';subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-Wall','-Wextra','-Werror','-I',str(root/'include')]+sources+[str(root/'tests/test_media.c'),'-o',str(exe),'-lm'],check=True);subprocess.run([str(exe)],cwd=t,check=True)
