static bool commit_pack(void){
 const char*bin=ROOT "/covers.bin",*idx=ROOT "/manifest.tsv";
 const char*oldbin=ROOT "/covers.bin.previous",*oldidx=ROOT "/manifest.tsv.previous";
 SceIoStat st;bool hadbin=sceIoGetstat(bin,&st)>=0,hadidx=sceIoGetstat(idx,&st)>=0;
 /* Backups remain until both new files are in place. */
 if(hadbin){sceIoRemove(oldbin);if(sceIoRename(bin,oldbin)<0)return false;}
 if(hadidx){sceIoRemove(oldidx);if(sceIoRename(idx,oldidx)<0){if(hadbin)sceIoRename(oldbin,bin);return false;}}
 if(sceIoRename(ROOT "/covers.bin.new",bin)<0){if(hadbin)sceIoRename(oldbin,bin);if(hadidx)sceIoRename(oldidx,idx);return false;}
 if(sceIoRename(ROOT "/manifest.tsv.new",idx)<0){sceIoRemove(bin);if(hadbin)sceIoRename(oldbin,bin);if(hadidx)sceIoRename(oldidx,idx);return false;}
 return sceIoSync("ux0:",0)>=0;
}
