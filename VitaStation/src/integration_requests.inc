extern "C" void store_integration_status(IntegrationStatus*out){if(!out)return;if(lock_id<0){*out={};snprintf(out->message,sizeof(out->message),"Servico de integracoes indisponivel.");return;}lock();*out=integration_status;unlock();}
extern "C" bool store_autoplugin_item(int index,OnlinePlugin*out){if(!out || lock_id<0)return false;lock();bool ok=index>=0 && (size_t)index<online_plugins.size();if(ok)*out=online_plugins[index];unlock();return ok;}
extern "C" void store_autoplugin_request(int index){
 if(lock_id<0){vs_log("integration-request","Store mutex unavailable",index);return;}
 // The packaged/cache index is read-only and must not wait behind network jobs.
 if(index==-3 && load_autoplugin_catalog())return;
 if(index==-3)index=-1;
 if(integration_thread_id<0){integration_message("Servico de plugins indisponivel. Reabre VitaStation.");return;}
 lock();if(integration_status.busy || update_busy || game_status.busy || progress.active || count){snprintf(integration_status.message,sizeof(integration_status.message),"Outra instalacao/integracao esta ativa. Aguarda e tenta novamente.");unlock();vs_log("integration-request","busy",index);return;}
 integration_status.busy=true;integration_pending=true;integration_action=index;snprintf(integration_status.message,sizeof(integration_status.message),"A consultar o AutoPlugin II...");unlock();sceKernelSignalSema(integration_wake_id,1);
}
extern "C" void store_apollo_request(){
 if(lock_id<0){vs_log("integration-request","Apollo: store unavailable",-2);return;}
 bool installed=pkgi_file_exists("ux0:app/NP0APOLLO/sce_sys/param.sfo");
 lock();if(integration_status.busy || update_busy || game_status.busy || progress.active || count){snprintf(integration_status.message,sizeof(integration_status.message),"Outra instalacao/integracao esta ativa. Aguarda e tenta novamente.");unlock();vs_log("integration-request","Apollo: busy",-2);return;}
 if(installed){apollo_ready=true;snprintf(integration_status.message,sizeof(integration_status.message),"A abrir o Apollo instalado...");unlock();vs_log("integration-request","Apollo installed: direct handoff",0);return;}
 if(integration_thread_id<0){snprintf(integration_status.message,sizeof(integration_status.message),"Instalador Apollo indisponivel. Reabre VitaStation.");unlock();return;}
 integration_status.busy=true;integration_pending=true;integration_action=-2;snprintf(integration_status.message,sizeof(integration_status.message),"A preparar a instalacao do Apollo...");unlock();sceKernelSignalSema(integration_wake_id,1);
}
extern "C" bool store_apollo_ready(){if(lock_id<0)return false;lock();bool ready=apollo_ready;apollo_ready=false;unlock();return ready;}
