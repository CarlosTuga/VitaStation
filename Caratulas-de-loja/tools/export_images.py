"""Export image assets from both the old per-file and new sequential cover VPKs."""
from pathlib import Path
import zipfile,sys,re,zlib
archive,out=map(Path,sys.argv[1:]);out.mkdir(parents=True,exist_ok=True)
def save(name,data):
 if not re.fullmatch(r'[A-Za-z0-9_][A-Za-z0-9_.-]{0,89}\.(png|jpg)',name) or '..' in name or len(data)>8*1024*1024:raise ValueError('Invalid image entry')
 target=out/name
 if target.exists() and target.read_bytes()!=data:raise ValueError('Existing different file preserved: '+str(target))
 target.write_bytes(data)
with zipfile.ZipFile(archive) as z:
 if 'assets/covers.bin' in z.namelist():
  lines=z.read('assets/manifest.tsv').decode('ascii').splitlines()
  with z.open('assets/covers.bin') as payload:
   for line in lines:
    name,size,crc=line.split();size=int(size)
    if size<=0 or size>8*1024*1024:raise ValueError('Invalid image size')
    data=payload.read(size)
    if len(data)!=size or zlib.crc32(data)!=int(crc,16):raise ValueError('Payload checksum mismatch')
    save(name,data)
   if payload.read(1):raise ValueError('Unexpected payload suffix')
 else:
  for item in z.infolist():
   if item.filename.startswith('assets/covers/'):
    if item.file_size>8*1024*1024:raise ValueError('Invalid image size')
    save(item.filename[len('assets/covers/'):],z.read(item))
print('Image payload exported without changing originals.')
