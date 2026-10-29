#include "launch_handoff.h"
#include "launcher.h"
#include "psp_media.h"
#include "log.h"
#include "bootstrap.h"
#include <stdio.h>
#include <string.h>

#if defined(__vita__)
#include <psp2/appmgr.h>
#endif

static LauncherResult last_result = {LAUNCH_PLATFORM_UNAVAILABLE, 0};
static bool exit_required=false;
bool launcher_exit_required(void){return exit_required;}

bool launcher_title_id_valid(const char *id) {
    if (!id) return false;
    for (unsigned i=0;i<9;i++) {
        unsigned char c=(unsigned char)id[i];
        if (!((c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9'))) return false;
    }
    return id[9]=='\0';
}

LauncherResult launcher_request_app(const char *id,const LauncherBackend *backend) {
    LauncherResult result={LAUNCH_INVALID_ID,0};
    if (!launcher_title_id_valid(id)) return result;
    if (!backend || !backend->is_installed || !backend->launch_uri) {
        result.status=LAUNCH_PLATFORM_UNAVAILABLE;return result;
    }
    if (!backend->is_installed(id,backend->context)) {
        result.status=LAUNCH_NOT_INSTALLED;return result;
    }
    char uri[64];
    int length=snprintf(uri,sizeof(uri),"psgm:play?titleid=%s",id);
    if (length<0 || (size_t)length>=sizeof(uri)) return result;
    /* SDK contract: 0x20000 requests launch rather than opening LiveArea. */
    result.platform_code=backend->launch_uri(0x20000,uri,backend->context);
    result.status=result.platform_code<0?LAUNCH_REJECTED:LAUNCH_ACCEPTED;
    return result;
}

#if defined(__vita__)
static bool vita_is_installed(const char *id,void *context) {
    (void)context;
    char path[100];
    snprintf(path,sizeof(path),"ux0:app/%s/sce_sys/param.sfo",id);
    FILE *f=fopen(path,"rb");
    if(!f && !strncmp(id,"NPXS",4)){snprintf(path,sizeof(path),"vs0:app/%s/sce_sys/param.sfo",id);f=fopen(path,"rb");}
    if (!f) return psp_native_installed(id);
    return fclose(f)==0;
}
static int vita_launch_uri(int flags,const char *uri,void *context) {
    (void)context;
    const char*prefix="psgm:play?titleid=";
    if(!strncmp(uri,prefix,strlen(prefix)) && launcher_title_id_valid(uri+strlen(prefix))){
        /* The shell opens the target only after this process has exited. */
        if(!bootstrap_busy() && shell_handoff_request("VTST00001",uri+strlen(prefix))){exit_required=true;return 0;}
        /* A missing/reloading bridge or optional preparation must not disable
           ordinary app launch. Let the OS own the caller lifecycle here. */
        return sceAppMgrLaunchAppByUri(flags,uri);
    }
    return sceAppMgrLaunchAppByUri(flags,uri);
}
#endif

bool launcher_launch_app(const char *id) {
    exit_required=false;
#if defined(__vita__)
    const LauncherBackend backend={vita_is_installed,vita_launch_uri,NULL};
    last_result=launcher_request_app(id,&backend);
#else
    last_result=launcher_request_app(id,NULL); /* A PC never claims to launch a Vita app. */
#endif
    char message[128];
    snprintf(message,sizeof(message),"title=%.9s status=%d api=0x%08X",id?id:"(null)",(int)last_result.status,(unsigned)last_result.platform_code);
    vs_log("launcher",message,last_result.platform_code);
    return last_result.status==LAUNCH_ACCEPTED;
}

LauncherResult launcher_last_result(void) { return last_result; }
const char *launcher_error_message(void) {
    switch(last_result.status) {
        case LAUNCH_INVALID_ID:return "O identificador desta aplicacao e invalido.";
        case LAUNCH_NOT_INSTALLED:return "Esta aplicacao ja nao esta disponivel. Atualiza a biblioteca.";
        case LAUNCH_PLATFORM_UNAVAILABLE:return "O lancamento so esta disponivel na PS Vita.";
        case LAUNCH_REJECTED:return last_result.platform_code==-0x7001?"Reinicia para carregar o plugin de abertura sem confirmacao.":"A consola nao aceitou o pedido de abertura.";
        default:return "Pedido de abertura enviado.";
    }
}
