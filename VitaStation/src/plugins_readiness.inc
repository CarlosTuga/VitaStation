/* Read-only readiness check. Never require rehashing the entire baseline when
   no installed module or configuration needs to be changed. */
static bool configured_bundle_current(){try{
    const std::string root=exists("ux0:tai/config.txt")?"ux0:tai":"ur0:tai";
    auto bytes=read_file(root+"/config.txt");std::string text(bytes.begin(),bytes.end());if(text.find('\0')!=std::string::npos)return false;
    std::vector<PluginEntry> entries(256);size_t count=plugins_config_entries(text.c_str(),entries.data(),entries.size());
    std::vector<BundledPlugin> all={{"VitaStationBoot.suprx","*main",VS_BOOT_SHA256}};for(auto&p:bundled_plugins)all.push_back(p);
    for(auto&p:all){bool disabled=false,found=false;
        for(size_t i=0;i<count;i++){const char*name=strrchr(entries[i].path,'/');name=name?name+1:entries[i].path;if(strcmp(name,p.file))continue;
            if(!entries[i].enabled){disabled=true;continue;}if(strcmp(entries[i].section,p.section) || !exists(entries[i].path))return false;
            std::string path=entries[i].path;auto colon=path.find(":/");if(colon!=std::string::npos)path.erase(colon+1,1);
            if((!strcmp(p.file,"VitaStationBoot.suprx") || path==root+"/VitaStation/"+p.file) && !verify_hash(read_file(entries[i].path),p.sha256))return false;
            found=true;
        }
        if(!disabled && !found)return false;
    }
    const std::string psp="ux0:pspemu/seplugins";bool original=exists(psp+"/npdrm_free.prx");std::string filename=original?"npdrm_free.prx":"npdrm_free_mod.prx";
    if(!exists(psp+"/"+filename) || (!original && !verify_hash(read_file(psp+"/"+filename),psp_plugin_hash)))return false;
    for(const char*name:{"game.txt","vsh.txt"}){if(!exists(psp+"/"+name))return false;auto data=read_file(psp+"/"+name);std::string input(data.begin(),data.end());std::vector<char> output(128*1024);if(plugins_psp_enable(input.c_str(),("ms0:/seplugins/"+filename).c_str(),output.data(),output.size())!=0)return false;}
    return true;
}catch(...){return false;}}
