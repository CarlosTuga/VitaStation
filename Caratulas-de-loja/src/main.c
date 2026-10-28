/* VitaStation offline cover installer. MIT. No network, plugin or firmware edits. */
#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/io/devctl.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/display.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <zlib.h>
#define ROOT "ux0:data/VitaStation/covers"
static vita2d_pgf*font;
static unsigned total,done,skipped,failed;
static uint64_t total_bytes,copy_start,copy_stop;
static unsigned char buffer[65536];
static void draw(const char*message,const char*detail){
 vita2d_start_drawing();vita2d_clear_screen();vita2d_draw_rectangle(0,0,960,544,RGBA8(12,22,40,255));
 vita2d_pgf_draw_text(font,42,78,RGBA8(245,248,255,255),1.5f,"CarÃ¡tulas de loja");
 vita2d_pgf_draw_text(font,42,124,RGBA8(180,201,225,255),.8f,"VitaStation | Imagens permanentes das duas lojas");
 vita2d_pgf_draw_text(font,42,196,RGBA8(245,248,255,255),1.0f,message);
 char short_detail[100];snprintf(short_detail,sizeof(short_detail),"%.95s",detail?detail:"");
 vita2d_pgf_draw_text(font,42,236,RGBA8(180,201,225,255),.7f,short_detail);
 char status[128];snprintf(status,sizeof(status),"%u / %u  |  JÃ¡ instaladas: %u  |  Falhas: %u",done,total,skipped,failed);
 vita2d_pgf_draw_text(font,42,306,RGBA8(245,248,255,255),.8f,status);
 if(copy_start){unsigned seconds=(unsigned)(((copy_stop?copy_stop:sceKernelGetProcessTimeWide())-copy_start)/1000000);char elapsed[96];snprintf(elapsed,sizeof(elapsed),"Tempo: %u:%02u | Cópia em bloco",seconds/60,seconds%60);vita2d_pgf_draw_text(font,42,328,RGBA8(180,201,225,255),.65f,elapsed);}
 vita2d_draw_rectangle(42,339,876,5,RGBA8(45,66,93,255));
 if(total)vita2d_draw_rectangle(42,339,876.0f*(float)done/total,5,RGBA8(226,239,255,255));
 vita2d_pgf_draw_text(font,42,421,RGBA8(180,201,225,255),.75f,"Destino: ux0:data/VitaStation/covers/");
 vita2d_pgf_draw_text(font,42,462,RGBA8(180,201,225,255),.75f,"As imagens mantÃªm-se apÃ³s reiniciar a consola.");
 vita2d_pgf_draw_text(font,42,515,RGBA8(140,165,195,255),.6f,"Imagens: HexFlow / PS Vita Alive e autores | CrÃ©ditos incluÃ­dos");
 vita2d_end_drawing();vita2d_swap_buffers();sceDisplayWaitVblankStart();
}
/* Shared native installer operations; exercised by host I/O mocks. */
static bool safe_name(const char*s){size_t n=strlen(s);if(n<5 || n>94 || s[0]=='.' || strstr(s,".."))return false;for(size_t i=0;i<n;i++)if(!isalnum((unsigned char)s[i]) && s[i]!='_' && s[i]!='-' && s[i]!='.')return false;return !strcmp(s+n-4,".png") || !strcmp(s+n-4,".jpg");}
static bool parse(char*line,char*name,uint64_t*size,uint32_t*crc){unsigned long long bytes;unsigned sum;char extra;if(sscanf(line,"%94s %llu %x %c",name,&bytes,&sum,&extra)!=3 || !safe_name(name) || !bytes || bytes>8*1024*1024)return false;*size=bytes;*crc=sum;return true;}
static bool marker(){const char*path=ROOT "/offline.enabled",*temp=ROOT "/offline.enabled.new";FILE*f=fopen(temp,"wb");if(!f)return false;bool ok=fputs("VitaStation permanent offline covers v1\n",f)>=0;if(fclose(f))ok=false;if(!ok)return false;SceIoStat st;if(sceIoGetstat(path,&st)>=0){sceIoRemove(temp);return true;}return sceIoRename(temp,path)>=0;}

#include "block_commit.h"
int main(void){
 vita2d_init();font=vita2d_load_default_pgf();if(!font)return 1;sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);
 sceIoMkdir("ux0:data",0777);sceIoMkdir("ux0:data/VitaStation",0777);sceIoMkdir(ROOT,0777);
 FILE*f=fopen("app0:assets/manifest.tsv","rb");char line[256],name[96];uint64_t size;uint32_t crc;
 bool valid=f!=NULL;
 if(f){while(fgets(line,sizeof(line),f)){if(!parse(line,name,&size,&crc)){valid=false;break;}total++;total_bytes+=size;}if(ferror(f) || !total)valid=false;rewind(f);}
 if(!valid){if(f)fclose(f);draw("Manifesto inválido.","Reinstala este VPK.");sceKernelDelayThread(3000000);goto end;}
 uint32_t previous=0;bool start=false;
 while(!start){draw("Instalação rápida em bloco | Requer VitaStation 1.5.7", "X: instalar | O: sair | Não elimina carátulas anteriores");SceCtrlData pad;sceCtrlPeekBufferPositive(0,&pad,1);uint32_t pressed=pad.buttons&~previous;previous=pad.buttons;if(pressed&SCE_CTRL_CIRCLE){fclose(f);goto end;}if(pressed&SCE_CTRL_CROSS)start=true;}
 SceIoDevInfo space;if(sceIoDevctl("ux0:",0x3001,NULL,0,&space,sizeof(space))>=0 && space.free_size>=0 && (uint64_t)space.free_size<total_bytes+4*1024*1024){fclose(f);draw("Espaço insuficiente para o bloco de imagens.","São necessários cerca de 250 MiB livres.");sceKernelDelayThread(4000000);goto end;}
 SceUID in=sceIoOpen("app0:assets/covers.bin",SCE_O_RDONLY,0),out=-1;
 FILE*index=fopen(ROOT "/manifest.tsv.new","wb");
 if(in>=0 && index)out=sceIoOpen(ROOT "/covers.bin.new",SCE_O_WRONLY|SCE_O_CREAT|SCE_O_TRUNC,0777);
 bool canceled=false;copy_start=sceKernelGetProcessTimeWide();uint64_t last_draw=0;
 if(in<0 || out<0 || !index)failed++;
 while(!failed && fgets(line,sizeof(line),f)){
  char original[256];snprintf(original,sizeof(original),"%s",line);if(!parse(line,name,&size,&crc)){failed++;break;}
  sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);SceCtrlData pad;sceCtrlPeekBufferPositive(0,&pad,1);if(pad.buttons&SCE_CTRL_CIRCLE){canceled=true;break;}
  uint64_t remaining=size;uLong actual=crc32(0L,Z_NULL,0);
  while(remaining){unsigned wanted=remaining>sizeof(buffer)?sizeof(buffer):(unsigned)remaining;int n=sceIoRead(in,buffer,wanted);if(n<=0){failed++;break;}actual=crc32(actual,buffer,n);int at=0;while(at<n){int wrote=sceIoWrite(out,buffer+at,n-at);if(wrote<=0){failed++;break;}at+=wrote;}if(failed)break;remaining-=n;}
  if(failed || actual!=crc || fputs(original,index)<0){failed++;break;}done++;
  uint64_t now=sceKernelGetProcessTimeWide();if(now-last_draw>=150000 || done==total){draw("A copiar o bloco de carátulas... O: parar",name);last_draw=now;}
 }
 if(ferror(f))failed++;
 if(in>=0){unsigned char extra;if(!canceled && !failed && sceIoRead(in,&extra,1)!=0)failed++;sceIoClose(in);}
 if(out>=0 && sceIoClose(out)<0)failed++;
 if(index && fclose(index))failed++;
 fclose(f);
 if(!failed && !canceled && done==total){if(sceIoSync("ux0:",0)<0 || !commit_pack() || !marker())failed++;}
 else if(!canceled && !failed)failed++;
 if(failed || canceled){sceIoRemove(ROOT "/covers.bin.new");sceIoRemove(ROOT "/manifest.tsv.new");}
 copy_stop=sceKernelGetProcessTimeWide();FILE*log=fopen(ROOT "/pack-install.log","a");if(log){fprintf(log,"BLOCK 1.04 DONE %u/%u failed %u canceled %d elapsed_ms=%llu\n",done,total,failed,canceled,(unsigned long long)((copy_stop-copy_start)/1000));fclose(log);}
 previous=0;while(1){draw(canceled?"Cópia interrompida; podes voltar a tentar.":failed?"Falha na instalação; consulta pack-install.log.":"Carátulas instaladas. Abre o VitaStation 1.5.7.","O: sair | As carátulas anteriores foram preservadas.");SceCtrlData pad;sceCtrlPeekBufferPositive(0,&pad,1);uint32_t pressed=pad.buttons&~previous;previous=pad.buttons;if(pressed&SCE_CTRL_CIRCLE)break;}
end:
 vita2d_wait_rendering_done();vita2d_free_pgf(font);vita2d_fini();sceKernelExitProcess(0);return 0;
}
