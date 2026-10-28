#include "system_audit.h"
#include "system_controls.h"
#include "system_info.h"
#include "settings_menu.h"
#include "settings.h"
#include "log.h"
#include <stdio.h>
void system_settings_audit(void){
 FILE*f=fopen(VS_DATA_DIR "/console-settings-audit.tsv","wb");if(!f)return;
 fputs("categoria\topcao\testado\testado_como\tdetalhe\n",f);unsigned available=0,unsupported=0,failed=0,actions=0;
 int page_count[128];for(unsigned i=0;i<128;i++)page_count[i]=-1;
 for(size_t i=0;i<system_feature_count();i++){
  if(settings_group_for((int)i+UI_LOCAL_SETTINGS_COUNT)!=SETTINGS_CONSOLE)continue;
  const SystemFeature*feature=system_feature_get(i);const char*state,*kind,*detail;
  if(feature->control<0){state="sem adaptador";kind="indisponivel";detail=system_feature_reason(feature);unsupported++;}
  else if(feature->control<=SYS_BRIGHTNESS){SystemControlResult result=system_control_read((SystemControl)feature->control);bool ok=result.status==SYS_APPLIED || result.status==SYS_BRIGHTNESS_SAVED;state=ok?"leitura confirmada":"falha de leitura";kind="controlo editavel; diagnostico nao altera valores";detail=system_control_message(result.status);if(ok)available++;else failed++;}
  else if(feature->control>=100 && feature->control<128){int page=feature->control;if(page_count[page]<0){char lines[6][128];page_count[page]=(int)system_info_read(page,lines);}bool ok=page_count[page]>0;state=ok?"consulta confirmada":"consulta indisponivel";kind="apenas leitura";detail=ok?"API devolveu dados; nao confirma edicao":"API nao devolveu dados";if(ok)available++;else failed++;}
  else{state="nao executada";kind="acao do sistema";detail="O diagnostico nao suspende, reinicia nem altera a consola";actions++;}
  fprintf(f,"%s\t%s\t%s\t%s\t%s\n",feature->category,feature->name,state,kind,detail);
 }
 fprintf(f,"TOTAL\tconsultas/controlos com leitura=%u\tsem adaptador=%u\tfalhas=%u\tacoes nao executadas=%u\n",available,unsupported,failed,actions);fclose(f);
 char message[160];snprintf(message,sizeof(message),"console settings: readable=%u unsupported=%u failed=%u actions=%u",available,unsupported,failed,actions);vs_log("settings-audit",message,failed?-1:0);
}
