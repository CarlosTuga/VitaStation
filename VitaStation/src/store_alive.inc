/* Typed Homebrew payloads. Files stage first; an unsuccessful commit restores originals. */
static std::string data_destination(const CatalogPackage&p){
    std::string path=p.extract_path;
    while(!path.empty() && path.back()=='/')path.pop_back();
    if(path=="ux0:app" && strlen(p.title_id)==9)path+="/"+std::string(p.title_id);
    const bool data=path.compare(0,9,"ux0:data/")==0 || path.compare(0,9,"ur0:data/")==0 || path=="ux0:data" || path=="ur0:data";
    const std::string app=std::string("ux0:app/")+p.title_id;
    const bool ownapp=p.title_id[0] && (path==app || path.compare(0,app.size()+1,app+"/")==0);
    if((!data && !ownapp) || alive_mediafire::toLowerAscii(path).find("vitastation")!=std::string::npos || !safe_relative(path.substr(4)))throw std::runtime_error("Destino de dados invalido ou protegido.");
    return path;
}
static void copy_file(const std::string&from,const std::string&to){
    SceUID in=sceIoOpen(from.c_str(),SCE_O_RDONLY,0);check(in,"Open source");
    SceUID out=sceIoOpen(to.c_str(),SCE_O_WRONLY|SCE_O_CREAT|SCE_O_EXCL,0777);
    if(out<0){sceIoClose(in);check(out,"Create data file");}
    try{uint8_t buffer[64*1024];int n;while((n=sceIoRead(in,buffer,sizeof(buffer)))>0){if(cancel_job || stop_worker)throw std::runtime_error("Download canceled");int at=0;while(at<n){int written=sceIoWrite(out,buffer+at,n-at);if(written<=0)throw std::runtime_error("Cannot save data file");at+=written;}}
        check(n,"Read data file");check(sceIoSyncByFd(out,0),"Sync data file");sceIoClose(out);sceIoClose(in);
    }catch(...){sceIoClose(out);sceIoClose(in);sceIoRemove(to.c_str());throw;}
}
static void merge_homebrew_data(const std::string&root,const std::string&destination){
    std::vector<MergeFile> files;collect_merge(root,"",files);if(files.empty())throw std::runtime_error("Nao existem ficheiros para instalar.");
    const std::string suffix=".VitaStation-"+std::to_string(sceKernelGetProcessTimeWide());
    std::vector<std::string> prepared;
    try{
        for(const auto&file:files){auto target=destination+"/"+file.relative;
            if(alive_mediafire::toLowerAscii(target).compare(0,20,"ux0:data/vitastation")==0 || alive_mediafire::toLowerAscii(target).compare(0,20,"ur0:data/vitastation")==0)throw std::runtime_error("Os ficheiros internos do launcher estao protegidos.");
            pkgi_mkdirs(target.substr(0,target.find_last_of('/')).c_str());
            if(pkgi_get_inode_type(target)==InodeType::Directory)throw std::runtime_error("O destino e uma pasta.");
            copy_file(root+"/"+file.relative,target+suffix+".new");prepared.push_back(target+suffix+".new");}
        for(auto&file:files){auto target=destination+"/"+file.relative;if(pkgi_file_exists(target)){pkgi_rename(target,target+suffix+".bak");file.old=true;}pkgi_rename(target+suffix+".new",target);file.moved=true;}
    }catch(...){bool restored=true;for(auto i=files.rbegin();i!=files.rend();++i){try{auto target=destination+"/"+i->relative;if(i->moved)check(sceIoRemove(target.c_str()),"Rollback new file");if(i->old)pkgi_rename(target+suffix+".bak",target);}catch(...){restored=false;}}
        for(const auto&path:prepared)sceIoRemove(path.c_str());
        if(!restored)throw std::runtime_error("Recuperacao incompleta; backups .VitaStation preservados no destino.");
        throw;}
    /* Keep overwritten files as recoverable .bak files, on the same partition. */
}
extern "C" bool plugins_install_catalog(const char*staged,const char*filename,const char*section,char*message,size_t capacity);
static void install_homebrew_payload(const CatalogPackage&p,const std::string&root){
    pkgi_delete_dir(root);pkgi_mkdirs(root.c_str());const auto archive=root+"/download";
    std::string destination;
    if(p.format!=CATALOG_LINK_PLUGIN)destination=data_destination(p);
    download_file(p,archive);
    if(p.format==CATALOG_LINK_PLUGIN){char error[256];if(!plugins_install_catalog(archive.c_str(),p.filename,p.plugin_section,error,sizeof(error)))throw std::runtime_error(error);phase(STORE_INSTALLING,"Plugin configurado. Reinicia a consola para aplicar.");return;}
    const auto content=root+"/content";pkgi_mkdirs(content.c_str());
    if(p.format==CATALOG_LINK_DATA_ZIP)extract_archive(archive,content);
    else if(p.format==CATALOG_LINK_DATA_FILE && safe_relative(p.filename) && !strchr(p.filename,'/'))pkgi_rename(archive,content+"/"+p.filename);
    else throw std::runtime_error("Formato de dados indisponivel.");
    phase(STORE_INSTALLING,"A instalar os ficheiros adicionais...");
    auto payload=content;
    if(destination.compare(0,8,"ux0:app/")==0 && pkgi_get_inode_type(content+"/"+p.title_id)==InodeType::Directory)payload+="/"+std::string(p.title_id);
    merge_homebrew_data(payload,destination);
}
