from pathlib import Path
import subprocess,tempfile,sys,zlib
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);source=t/'inventory.c'
 source.write_text('#include "cover_pack.h"\n#include "cover_inventory.h"\n#include <assert.h>\n#include <stdlib.h>\nint main(int argc,char**argv){assert(argc==2);assert(VS_COVER_EXPECTED_COUNT==21811u);assert(cover_pack_installed_count()==strtoul(argv[1],0,10));return 0;}')
 exe=t/'inventory.exe'
 subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-Wall','-Wextra','-Werror','-DVS_COVER_PACK_ROOT="."','-I',str(r/'include'),str(r/'src/cover_pack.c'),str(source),'-o',str(exe)],check=True)
 def check(rows,data,expected):
  (t/'manifest.tsv').write_text(rows);(t/'covers.bin').write_bytes(data)
  subprocess.run([str(exe),str(expected)],cwd=t,check=True)
 crc=zlib.crc32(b'x')
 def row(name):return f'{name}\t1\t{crc:08x}\n'
 check(row('a.jpg'),b'x',1)
 check(row('a.jpg')+row('a.jpg'),b'xx',0)
 check(row('a.jpg')+row('b.jpg'),b'x',0)
 check(row('a.jpg')+'invalid\n',b'x',0)
 check(row('../a.jpg'),b'x',0)
 check(''.join(row(f'{i:05}.jpg') for i in range(21811)),b'x'*21811,21811)
 print('Cover inventory: expected 21811, partial count, duplicates, truncation, malformed/traversal and complete manifest passed.')
