/* Native OpenSSL store from individual DER roots; no filesystem or PEM parser. */
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "ca_bundle.h"
static std::mutex ca_store_mutex;
static X509_STORE*ca_store=nullptr;
static void prepare_ca_store(){
    X509_STORE*store=X509_STORE_new();if(!store)throw std::runtime_error("Sem memoria para os certificados HTTPS.");
    try{
        unsigned imported=0;
        for(const auto&root:vs_ca_roots){
            const unsigned char*at=vs_ca_der+root.offset;ERR_clear_error();
            X509*cert=d2i_X509(nullptr,&at,root.length);
            if(!cert || at!=vs_ca_der+root.offset+root.length){if(cert)X509_free(cert);unsigned long code=ERR_get_error();char text[160];ERR_error_string_n(code,text,sizeof(text));throw std::runtime_error(fmt::format("Raiz {}: {} ({:08x})",imported,text,(unsigned)code));}
            int added=X509_STORE_add_cert(store,cert);X509_free(cert);
            if(added!=1)throw std::runtime_error(fmt::format("Falha ao importar raiz {}: {:08x}",imported,(unsigned)ERR_get_error()));
            imported++;
        }
        char message[128];snprintf(message,sizeof(message),"VS1.5.6: %u DER trust roots loaded; %s",imported,OpenSSL_version(OPENSSL_VERSION));vs_log("https-ca",message,0);
        ca_store=store;
    }catch(...){X509_STORE_free(store);throw;}
}
static CURLcode attach_ca_store(CURL*,void*context,void*){
    try{
        /* Vita's libstdc++ call_once throws system_error here (observed on 3.65).
           std::mutex is already used successfully by init_network on this device. */
        std::lock_guard<std::mutex> guard(ca_store_mutex);
        if(!ca_store)prepare_ca_store();
        if(!ca_store || X509_STORE_up_ref(ca_store)!=1)return CURLE_OUT_OF_MEMORY;
        SSL_CTX_set_cert_store(static_cast<SSL_CTX*>(context),ca_store);
        return CURLE_OK;
    }catch(const std::system_error&e){char detail[192];snprintf(detail,sizeof(detail),"Sincronizacao CA: %s; codigo=%d categoria=%s",e.what(),e.code().value(),e.code().category().name());vs_log("https-ca",detail,e.code().value());return CURLE_SSL_CERTPROBLEM;}
    catch(const std::exception&e){vs_log("https-ca",e.what(),-1);return CURLE_SSL_CERTPROBLEM;}
}
static std::string transfer_error(CURLcode code,const char*detail){
    std::string message=fmt::format("VS1.5.6 HTTPS {}: {}",(int)code,detail && *detail?detail:curl_easy_strerror(code));
    vs_log("https",message.c_str(),(int)code);return message;
}
