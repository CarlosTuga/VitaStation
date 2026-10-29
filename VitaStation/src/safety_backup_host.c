#include "safety_backup.h"
#include <stdio.h>
#if !defined(__vita__)
bool safety_backup_exists(void){return true;}
bool safety_backup_ensure(char*m,size_t n){if(m && n)snprintf(m,n,"Copia de plugins exige uma PS Vita.");return false;}
bool safety_backup_restore(char*m,size_t n){return safety_backup_ensure(m,n);}
bool safety_backup_track_plugin(const char*p,char*m,size_t n){(void)p;return safety_backup_ensure(m,n);}
bool safety_backup_remove_additions(char*m,size_t n){return safety_backup_ensure(m,n);}
size_t safety_backup_plan(char lines[][128],size_t capacity){if(!capacity)return 0;snprintf(lines[0],128,"Ficheiros de plugins preservados; configuracoes repostas na Vita.");return 1;}
#endif
