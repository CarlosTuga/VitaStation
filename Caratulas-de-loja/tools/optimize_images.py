from pathlib import Path
from concurrent.futures import ProcessPoolExecutor,as_completed
from PIL import Image
import io,json,hashlib,time,sys
source=Path(sys.argv[1]);target=Path(sys.argv[2]);target.mkdir(exist_ok=True,parents=True)
if source.resolve()==target.resolve():raise ValueError('Input and output directories must differ')
def optimize(path):
 p=Path(path);data=p.read_bytes()
 with Image.open(io.BytesIO(data)) as opened:
  im=opened.convert('RGBA');original_size=im.size;im.thumbnail((224,320),Image.Resampling.LANCZOS)
  alpha=im.getchannel('A').getextrema()[0]<255
  buffer=io.BytesIO()
  if alpha:
   im.save(buffer,'PNG',optimize=True);encoded=buffer.getvalue();stem=p.name.rsplit('.',1)[0];name=stem+'.png'
   if original_size==im.size and data.startswith(b'\x89PNG') and len(data)<len(encoded):encoded=data
  else:
   im.convert('RGB').save(buffer,'JPEG',quality=70,optimize=True,progressive=False);encoded=buffer.getvalue();stem=p.name.rsplit('.',1)[0]
   if stem.endswith('.cover'):stem=stem[:-6]
   name=stem+'.cover.jpg'
 out=target/name
 # Name collisions are equivalent lookups; retain the original PNG source over JPEG cache.
 return {'source':p.name,'name':name,'before':len(data),'after':len(encoded),'dimensions':im.size,'sha256':hashlib.sha256(encoded).hexdigest()},encoded
if __name__=='__main__':
 files=sorted(source.glob('*'),key=lambda p:(p.suffix.lower()!='.png',p.name));jobs={};results=[];start=time.time();last=start
 with ProcessPoolExecutor(max_workers=4) as pool:
  futures={pool.submit(optimize,str(p)):p for p in files}
  records={}
  for future in as_completed(futures):
   meta,data=future.result();p=futures[future];key=meta['name']
   # Priority independent of completion order.
   rank=(p.suffix.lower()!='.png',p.name)
   if key not in records or rank<records[key][0]:
    (target/key).write_bytes(data);records[key]=(rank,meta)
   if time.time()-last>15:print('OPTIMIZE',len(records),'images',round(time.time()-start),'s',flush=True);last=time.time()
 results=[v[1] for v in records.values()]
 report={'files':len(results),'source_files':len(files),'source_bytes':sum(p.stat().st_size for p in files),'optimized_bytes':sum(r['after'] for r in results),'max_dimensions':[224,320],'jpeg_quality':70,'transparent_images':'PNG, transparency retained','images':results}
 (target.parent/'optimization.json').write_text(json.dumps(report,indent=2),encoding='utf-8');print('DONE',len(results),'from',report['source_bytes'],'to',report['optimized_bytes'],'bytes',flush=True)
