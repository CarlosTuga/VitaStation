from pathlib import Path
import subprocess,tempfile,sys,zlib
p=Path('outputs/VitaStation').resolve()
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);images=[('PCSE00001.cover.jpg',b'first payload'),('hb_1234567890123456.png',b'second payload')];(t/'covers.bin').write_bytes(b''.join(b for _,b in images));(t/'manifest.tsv').write_text(''.join(f'{name}\t{len(b)}\t{zlib.crc32(b):08x}\n' for name,b in images))
 source=t/'test.c';source.write_text('''#include "cover_pack.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
int main(void){char key[256];size_t n=0;assert(cover_pack_find("hb_1234567890123456.png",key,sizeof(key)));unsigned char*b=cover_pack_read(key,&n);assert(b && n==14 && !memcmp(b,"second payload",14));free(b);assert(cover_pack_find("PCSE00001.cover.jpg",key,sizeof(key)));b=cover_pack_read(key,&n);assert(b && n==13 && !memcmp(b,"first payload",13));free(b);assert(!cover_pack_find("missing.png",key,sizeof(key)));assert(!cover_pack_read("vspack:0:99999999:0",&n));FILE*f=fopen("covers.bin","r+b");assert(f);fputc(0,f);fclose(f);assert(cover_pack_find("PCSE00001.cover.jpg",key,sizeof(key)));assert(!cover_pack_read(key,&n));puts("Pack index lookup, offsets, bounds and CRC corruption checks passed.");}
''')
 exe=t/'test.exe';subprocess.run([sys.executable,'-m','ziglang','cc','-std=c99','-Wall','-Wextra','-Werror','-DVS_COVER_PACK_ROOT="."','-I',str(p/'include'),str(p/'src/cover_pack.c'),str(source),'-o',str(exe)],check=True);subprocess.run([str(exe)],cwd=t,check=True)
