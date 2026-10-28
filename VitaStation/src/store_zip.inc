/* 64-bit Vita libzip source; inspired by PSVitaAlive (MIT). */
struct VitaZipSource {std::string path;SceUID fd=-1;zip_error_t error;uint64_t size=0,pos=0;};
static zip_int64_t vita_zip_read(void*context,void*data,zip_uint64_t length,zip_source_cmd_t cmd){
    auto*s=(VitaZipSource*)context;
    switch(cmd){
    case ZIP_SOURCE_SUPPORTS:return ZIP_SOURCE_SUPPORTS_SEEKABLE;
    case ZIP_SOURCE_OPEN:s->fd=sceIoOpen(s->path.c_str(),SCE_O_RDONLY,0);s->pos=0;return s->fd<0?-1:0;
    case ZIP_SOURCE_READ:{int n=sceIoRead(s->fd,data,(SceSize)std::min<uint64_t>(length,512*1024));if(n<0){zip_error_set(&s->error,ZIP_ER_READ,0);return -1;}s->pos+=n;return n;}
    case ZIP_SOURCE_CLOSE:if(s->fd>=0)sceIoClose(s->fd);s->fd=-1;return 0;
    case ZIP_SOURCE_STAT:{if(length<sizeof(zip_stat_t))return -1;auto*st=(zip_stat_t*)data;zip_stat_init(st);st->size=s->size;st->valid=ZIP_STAT_SIZE;return sizeof(*st);}
    case ZIP_SOURCE_SEEK:{auto offset=zip_source_seek_compute_offset(s->pos,s->size,data,length,&s->error);if(offset<0)return -1;auto result=sceIoLseek(s->fd,offset,SCE_SEEK_SET);if(result<0){zip_error_set(&s->error,ZIP_ER_SEEK,0);return -1;}s->pos=result;return 0;}
    case ZIP_SOURCE_TELL:return s->pos;
    case ZIP_SOURCE_ERROR:return zip_error_to_data(&s->error,data,length);
    case ZIP_SOURCE_FREE:if(s->fd>=0)sceIoClose(s->fd);zip_error_fini(&s->error);delete s;return 0;
    default:zip_error_set(&s->error,ZIP_ER_OPNOTSUPP,0);return -1;
    }
}
static zip_t* open_vita_zip(const std::string&path){
    SceIoStat st{};check(sceIoGetstat(path.c_str(),&st),"Read archive size");auto*s=new VitaZipSource;s->path=path;s->size=st.st_size;zip_error_init(&s->error);
    zip_error_t error;zip_error_init(&error);auto*source=zip_source_function_create(vita_zip_read,s,&error);
    if(!source){delete s;zip_error_fini(&error);throw std::runtime_error("Cannot open ZIP source");}
    auto*z=zip_open_from_source(source,ZIP_RDONLY,&error);zip_error_fini(&error);if(!z){zip_source_free(source);throw std::runtime_error("Invalid ZIP/VPK archive");}return z;
}
