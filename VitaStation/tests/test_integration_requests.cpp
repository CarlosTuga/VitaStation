#include "integrations.h"
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
#include <cassert>
static int lock_id=1,integration_thread_id=-1,integration_wake_id=42;
static IntegrationStatus integration_status{};
static std::vector<OnlinePlugin>online_plugins;
static bool integration_pending,apollo_ready,update_busy;
static int integration_action;
static struct{bool busy;}game_status;
static struct{bool active;}progress;
static unsigned count,signals,cache_reads;
static bool installed,cached,held;
static void lock(){assert(!held);held=true;}static void unlock(){assert(held);held=false;}
static void vs_log(const char*,const char*,int){}
static int sceKernelSignalSema(int id,int n){assert(id==42 && n==1);signals++;return 0;}
static bool pkgi_file_exists(const char*p){assert(!strcmp(p,"ux0:app/NP0APOLLO/sce_sys/param.sfo"));return installed;}
static bool load_autoplugin_catalog(){cache_reads++;if(cached)integration_status.count=140;return cached;}
static void integration_message(const std::string&s){snprintf(integration_status.message,sizeof(integration_status.message),"%s",s.c_str());}
#include "../src/integration_requests.inc"
static void reset(){integration_status={};integration_pending=apollo_ready=update_busy=false;integration_action=0;game_status.busy=progress.active=false;count=signals=cache_reads=0;installed=cached=held=false;integration_thread_id=-1;lock_id=1;}
int main(){
 reset();cached=true;update_busy=true;store_autoplugin_request(-3);assert(cache_reads==1 && integration_status.count==140 && !integration_pending && signals==0);
 reset();integration_thread_id=7;store_autoplugin_request(-3);assert(integration_pending && integration_action==-1 && integration_status.busy && signals==1);
 reset();integration_thread_id=7;update_busy=true;store_autoplugin_request(0);assert(!integration_pending && !signals && integration_status.message[0]);
 reset();store_autoplugin_request(0);assert(!integration_pending && !signals && integration_status.message[0]);
 reset();installed=true;store_apollo_request();assert(store_apollo_ready() && !store_apollo_ready() && signals==0 && !integration_status.busy);
 reset();store_apollo_request();assert(!apollo_ready && !integration_status.busy && integration_status.message[0]);
 reset();integration_thread_id=7;store_apollo_request();assert(integration_pending && integration_action==-2 && signals==1 && integration_status.busy);
 reset();installed=true;progress.active=true;store_apollo_request();assert(!apollo_ready && integration_status.message[0]);
 puts("Production integration requests: local catalogue bypasses worker/network queue; dedicated service signal, installed Apollo direct launch, missing service and active installer handling passed.");
}
