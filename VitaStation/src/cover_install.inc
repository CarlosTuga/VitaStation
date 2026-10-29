static void install_cover_archive(const std::string&archive){
    zip_t*z=open_vita_zip(archive);zip_file_t*input=nullptr;FILE*output=nullptr;
    try{
        zip_stat_t st{};if(zip_stat(z,"assets/manifest.tsv",0,&st)<0 || !st.size || st.size>2*1024*1024)throw std::runtime_error("Manifesto de caratulas invalido.");
        std::vector<char>manifest(st.size+1);input=zip_fopen(z,"assets/manifest.tsv",0);if(!input)throw std::runtime_error("Manifesto ausente.");
        size_t at=0;while(at<st.size){auto n=zip_fread(input,manifest.data()+at,st.size-at);if(n<=0)throw std::runtime_error("Manifesto incompleto.");at+=n;}
        if(zip_fclose(input)<0){input=nullptr;throw std::runtime_error("CRC do manifesto invalido.");}input=nullptr;
        struct Cover {uint32_t size,crc;};std::vector<Cover>images;std::set<std::string>names;uint64_t total=0;std::string text(manifest.data(),st.size);
        for(size_t pos=0;pos<text.size();){size_t end=text.find('\n',pos);if(end==std::string::npos)end=text.size();auto line=text.substr(pos,end-pos);pos=end+1;
            char name[96],extra;unsigned long long size;unsigned crc;
            if(line.empty() || line.size()>250 || sscanf(line.c_str(),"%95s %llu %x %c",name,&size,&crc,&extra)!=3 || !size || size>8*1024*1024 || images.size()>=32768 || !safe_relative(name) || strchr(name,'/') || strchr(name,'\\'))throw std::runtime_error("Entrada de caratula invalida.");
            if(!names.insert(name).second)throw std::runtime_error("Caratula repetida no manifesto.");
            total+=size;images.push_back({(uint32_t)size,crc});
        }
        if(images.size()!=VS_COVER_EXPECTED_COUNT || total!=VS_COVER_EXPECTED_BYTES || total>512*1024*1024 || zip_stat(z,"assets/covers.bin",0,&st)<0 || st.size!=total)throw std::runtime_error("Bloco de caratulas incompleto.");
        free_space(total);pkgi_mkdirs(ROOT);input=zip_fopen(z,"assets/covers.bin",0);if(!input)throw std::runtime_error("Bloco ausente.");
        output=fopen(ROOT "/covers.bin.new","wb");if(!output)throw std::runtime_error("Sem acesso a pasta das caratulas.");
        uint8_t buffer[65536];unsigned done=0;update_units(0,images.size());
        for(const auto&image:images){uint32_t left=image.size;uLong actual=crc32(0L,Z_NULL,0);
            while(left){if(stop_worker)throw std::runtime_error("Instalacao interrompida.");unsigned wanted=std::min<unsigned>(left,sizeof(buffer));auto n=zip_fread(input,buffer,wanted);if(n<=0 || fwrite(buffer,1,n,output)!=(size_t)n)throw std::runtime_error("Erro ao copiar caratulas.");actual=crc32(actual,buffer,n);left-=n;}
            if(actual!=image.crc)throw std::runtime_error("Caratula danificada: CRC incorreto.");
            if(++done%128==0 || done==images.size()){char message[160];snprintf(message,sizeof(message),"A instalar caratulas: %u / %u",done,(unsigned)images.size());update_message(message);update_units(done,images.size());sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);}
        }
        if(zip_fclose(input)<0){input=nullptr;throw std::runtime_error("CRC do bloco incorreto.");}input=nullptr;
        int closed=fclose(output);output=nullptr;if(closed)throw std::runtime_error("Falha ao guardar caratulas.");
        pkgi_save(ROOT "/manifest.tsv.new",manifest.data(),text.size());
        if(!commit_pack())throw std::runtime_error("Falha ao publicar caratulas; anteriores preservadas quando possivel.");
        zip_discard(z);z=nullptr;cover_pack_reload();
        if(cover_pack_installed_count()!=VS_COVER_EXPECTED_COUNT)throw std::runtime_error("Verificacao final de caratulas falhou.");
        const char marker[]="VitaStation permanent offline covers v1\n";pkgi_save(ROOT "/offline.enabled",marker,sizeof(marker)-1);
        lock();++progress.covers_revision;unlock();
    }catch(...){if(output)fclose(output);if(input)zip_fclose(input);if(z)zip_discard(z);sceIoRemove(ROOT "/covers.bin.new");sceIoRemove(ROOT "/manifest.tsv.new");throw;}
}
