"""Set SFO display names as UTF-8, independent of Windows shell codepage."""
from pathlib import Path
import struct,sys
p=Path(sys.argv[1]);b=bytearray(p.read_bytes());_,_,keys,values,count=struct.unpack_from('<5I',b)
name='Carátulas de loja'.encode('utf-8')+b'\0'
for i in range(count):
 at=20+i*16;ko,fmt,n,capacity,offset=struct.unpack_from('<HHIII',b,at);key=b[keys+ko:].split(b'\0',1)[0]
 if key in [b'TITLE',b'STITLE']:
  assert fmt==0x204 and len(name)<=capacity
  b[values+offset:values+offset+capacity]=name+b'\0'*(capacity-len(name));struct.pack_into('<I',b,at+4,len(name))
p.write_bytes(b)
