"""Package a separate offline artwork installer; no Vita network needed.
Usage: python tools/package_vpk.py BUILD_DIRECTORY IMAGE_DIRECTORY OUTPUT_VPK
"""
from pathlib import Path
import sys,zipfile,zlib,hashlib,json,re,time,struct
project=Path(__file__).resolve().parents[1]
build,images,output=map(Path,sys.argv[1:]);output=output.resolve();temporary=output.with_suffix('.vpk.part')
sfo=(build/'param.sfo').read_bytes();_,_,keys,values,count=struct.unpack_from('<5I',sfo);fields={}
for i in range(count):
 ko,fmt,n,m,offset=struct.unpack_from('<HHIII',sfo,20+i*16);key=sfo[keys+ko:].split(b'\0',1)[0].decode()
 if fmt==0x204:fields[key]=sfo[values+offset:values+offset+n].split(b'\0',1)[0].decode('utf-8')
assert fields['TITLE_ID']=='VSCV00001' and fields['APP_VER']=='01.04',fields
print('SFO',fields,flush=True)
files=sorted(f for f in images.iterdir() if f.is_file());manifest=[];inventory=[];start=time.time();last=start
for f in files:
 if not re.fullmatch(r'[A-Za-z0-9_][A-Za-z0-9_.-]{0,89}\.(?:png|jpg)',f.name) or '..' in f.name:raise ValueError('Unsafe image filename '+f.name)
 if f.stat().st_size>8*1024*1024 or f.stat().st_size==0:raise ValueError('Invalid image size')
with zipfile.ZipFile(temporary,'w',zipfile.ZIP_DEFLATED,compresslevel=1,allowZip64=False) as z:
 z.write(build/'CoverInstaller.self','eboot.bin');z.write(build/'param.sfo','sce_sys/param.sfo')
 z.write(project.parent/'VitaStation/assets/icon0.png','sce_sys/icon0.png')
 with z.open('assets/covers.bin','w',force_zip64=False) as payload:
  for i,f in enumerate(files,1):
   data=f.read_bytes()
   if not (data.startswith(b'\x89PNG\r\n\x1a\n') or data.startswith(b'\xff\xd8\xff')):raise ValueError('Invalid image signature '+f.name)
   manifest.append(f'{f.name}\t{len(data)}\t{zlib.crc32(data):08x}\n');inventory.append({'file':f.name,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
   payload.write(data)
   if time.time()-last>15 or i==len(files):print('PACK',i,'/',len(files),'seconds',round(time.time()-start),flush=True);last=time.time()
 text=''.join(manifest);z.writestr('assets/manifest.tsv',text)
 for f in sorted((project/'licenses').glob('*')):z.write(f,'assets/licenses/'+f.name)
 z.write(project/'LICENSE','assets/licenses/Installer-MIT.txt')
 for name in ['CREDITOS.txt','COBERTURA.json']:
  if (project/name).exists():z.write(project/name,'assets/'+name)
# Keep exact payload inventory with the source project, without a duplicate 740MiB image tree.
(project/'manifest.tsv').write_text(text,encoding='ascii');(project/'IMAGE_SHA256.json').write_text(json.dumps(inventory,indent=2),encoding='utf-8')
print('Verifying every ZIP entry...',flush=True)
with zipfile.ZipFile(temporary) as z:
 bad=z.testzip();assert bad is None,bad
 assert z.getinfo('assets/covers.bin').file_size==sum(item['bytes'] for item in inventory)
 with z.open('assets/covers.bin') as payload:
  for line in manifest:
   name,size,crc=line.split();data=payload.read(int(size));assert len(data)==int(size) and zlib.crc32(data)==int(crc,16),name
  assert not payload.read(1)
 assert z.read('assets/manifest.tsv').decode('ascii')==text
 if 'assets/CREDITOS.txt' not in z.namelist():raise ValueError('Credits missing')
temporary.replace(output)
print('READY',str(output),output.stat().st_size,'bytes;',len(files),'images',flush=True)
