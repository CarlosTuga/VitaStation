struct MergeFile{std::string relative;bool old=false,moved=false;};
static void collect_merge(const std::string&root,const std::string&relative,std::vector<MergeFile>&files){
    for(const auto&name:pkgi_list_dir_contents(relative.empty()?root:root+"/"+relative)){
        if(!safe_relative(name) || name.find('/')!=std::string::npos || name=="__VitaStation_previous")throw std::runtime_error("Unsafe PSP DLC path");
        auto path=relative.empty()?name:relative+"/"+name;
        if(pkgi_get_inode_type(root+"/"+path)==InodeType::Directory)collect_merge(root,path,files);
        else files.push_back({path,false,false});
        if(files.size()>65536)throw std::runtime_error("Too many PSP DLC files");
    }
}
