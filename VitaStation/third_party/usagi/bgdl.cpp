/* Adapted from Usagi PKGj (BSD-2-Clause), commit 744541320532b59335771de67399fdeb231f7284. */
#include "bgdl.hpp"
#include "bgdl_payload.h"
#include <cstddef>
#include "download.hpp"
#include "file.hpp"
#include "log.hpp"

#include <taihen.h>
#include <vitasdk.h>

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <mutex>
#include <cstdio>
extern "C" {
#include "log.h"
}

// RE by @dots_tb with help from:
//       @CelesteBlue123 @SilicaDevs @possvkey and the NPS Team

namespace
{
// offsets for DC0 structure
// 0x70 URL
// 0x870 icon path
// 0x970 title
// 0xCAA license path (the bgldl will copy and paste it to bgdlid folder)

typedef BgdlPayload ipmi_download_param;
static_assert(sizeof(BgdlPayload)==0xdc0,"ShellSvc wire payload size");
static_assert(offsetof(BgdlPayload,url)==0x70,"ShellSvc URL offset");
static_assert(offsetof(BgdlPayload,license_path)==0xcaa,"ShellSvc RIF offset");
typedef struct sce_ipmi_download_param
{
    union
    {
        struct
        {
            uint32_t* ptr_to_dc0_ptr;
            uint32_t* ptr_to_2e0_ptr;
            uint32_t unk_1; // 2
            uint32_t unk_2; //-1
            uint32_t unk_3; // 0
            ipmi_download_param* addr_DC0;
            uint32_t sizeDC0;
        } init;

        struct
        {
            int32_t* result;
            uint32_t unk_2; // 0
            uint32_t unk_3; // 0
            uint32_t unk_4; // 1
            uint32_t unk_5; // 0
            uint32_t unk_6;
            uint32_t unk_7; // 0x00000A0A
        } state;
    };
    void* addr_2E0;
    uint32_t size2E0;
    uint32_t unk_4;
    uint32_t* pBgdlId; // points to -1
    uint32_t unk_5; // 4
    int32_t* result; // points to 1
    uint32_t unk_4_2;
    uint32_t shell_func_8;
} sce_ipmi_download_param;

typedef struct ipmi_download_param_state
{
    int32_t* result;
    uint32_t unk_2; // 0
    uint32_t unk_3; // 0
    uint32_t unk_4; // 1
    uint32_t unk_5; // 0
    uint32_t unk_6;
    uint32_t unk_7; // 0x00000A0A
} ipmi_download_param_state;

typedef struct shellsvc_init_struct
{
    uint32_t unk_0;
    char name[0x10];
    void* unk_ptr;
    uint32_t unk_1; // 1
    uint32_t size1; // 0x1E00
    uint32_t size2; // 0x1E00
    uint32_t unk_2; // 1
    uint32_t unk_3; // 0x0F00
    uint32_t unk_4; // 0x0F00
    uint32_t unk_5; // 1
    char padding[0x84];
    uint32_t unk_7; // 2
    uint32_t unk_8; // -1
    void* unk_ptr_2;
    char padding2[0x88]; // 0x14c
} shellsvc_init_struct;

typedef struct scedownload_class_header
{
    uint32_t unk0;
    uint32_t unk1;
    uint32_t unk2;
    uint32_t** func_table;
    uint32_t unk3;
    uint32_t* bufC4;
    uint32_t* buf10000;
} scedownload_class_header;

typedef int (*SceDownloadInit)(
        uint32_t** ipmi_sce_download_ptr,
        void* ipmi_sce_download_ptr_deref,
        int unk_1,
        shellsvc_init_struct* bufc8,
        int unk_2);
typedef int (*SceDownloadChangeState)(
        uint32_t** ipmi_sce_download_ptr,
        int cmd,
        void* ptr_to_dc0_ptr,
        int unk_1,
        sce_ipmi_download_param r5);

typedef struct scedownload_class
{
    shellsvc_init_struct init_header{};
    scedownload_class_header* class_header=nullptr;
    SceDownloadInit init=nullptr;
    SceDownloadChangeState change_state=nullptr;
    ~scedownload_class(){if(class_header){delete[] class_header->bufC4;delete[] class_header->buf10000;delete class_header;}}
} scedownload_class;

// sceIpmiCreateDownloadTask?
int (*SceIpmi_4E255C31)(const char* name, int unk);

// sceIpmiInitDownloadTask?
int (*SceIpmi_B282B430)(
        uint32_t*** func_table,
        const char* name,
        scedownload_class_header* class_header,
        uint32_t* buf10000);

void init_download_class(scedownload_class* sceDownloadObj)
{

    strncpy(sceDownloadObj->init_header.name, "SceDownload", 0x10);
    sceDownloadObj->init_header.unk_1 = 1;
    sceDownloadObj->init_header.size1 = 0x1E00;
    sceDownloadObj->init_header.size2 = 0x1E00;
    sceDownloadObj->init_header.unk_2 = 1;
    sceDownloadObj->init_header.unk_3 = 0x0F00;
    sceDownloadObj->init_header.unk_4 = 0x0F00;
    sceDownloadObj->init_header.unk_5 = 1;
    sceDownloadObj->init_header.unk_7 = 2;
    sceDownloadObj->init_header.unk_8 = -1;

    int res = SceIpmi_4E255C31(
            (char*)&(sceDownloadObj->init_header.name), 0x1E00);

    if (res != 0xc4)
        throw formatEx<std::runtime_error>(
                "SceIpmi_4E255C31 failed: {:#08x}", static_cast<uint32_t>(res));

    sceDownloadObj->class_header = new scedownload_class_header{};

    sceDownloadObj->class_header->bufC4 = new uint32_t[((unsigned)res+3)/4]();

    sceDownloadObj->class_header->buf10000 = new uint32_t[0x1000/4]();

    res = SceIpmi_B282B430(
            &(sceDownloadObj->class_header->func_table),
            (char*)&(sceDownloadObj->init_header.name),
            sceDownloadObj->class_header,
            sceDownloadObj->class_header->buf10000);
    if (res != 0)
        throw formatEx<std::runtime_error>(
                "SceIpmi_B282B430 init failed: {:#08x}",
                static_cast<uint32_t>(res));

    if(!sceDownloadObj->class_header->func_table || !*sceDownloadObj->class_header->func_table)throw std::runtime_error("Invalid ShellSvc function table");
    sceDownloadObj->init =
            (SceDownloadInit)(*(sceDownloadObj->class_header->func_table))[1];
    sceDownloadObj->change_state = (SceDownloadChangeState)(*(
            sceDownloadObj->class_header->func_table))[5];

    if(!sceDownloadObj->class_header->func_table || !*sceDownloadObj->class_header->func_table || !sceDownloadObj->init || !sceDownloadObj->change_state)throw std::runtime_error("Invalid ShellSvc download interface");
    res = sceDownloadObj->init(
            sceDownloadObj->class_header->func_table,
            *(sceDownloadObj->class_header->func_table),
            0x14,
            &(sceDownloadObj->init_header),
            2);
    if (res != 0)
        throw formatEx<std::runtime_error>(
                "SceDownload init failed: {:#08x}", static_cast<uint32_t>(res));
}

unsigned scedownload_start_with_rif(
        scedownload_class* sceDownloadObj,
        const char* title,
        const char* url,
        const char* rif,
        int type, const std::string& icon)
{
    int32_t result = 0;
    int32_t bgdlid = -1;

    sce_ipmi_download_param params;
    memset(&params, 0, sizeof(params));

    params.init.ptr_to_dc0_ptr = (uint32_t*)&params.init.addr_DC0;
    params.init.ptr_to_2e0_ptr = (uint32_t*)&params.addr_2E0;
    params.init.unk_1 = 2;
    params.init.unk_2 = -1;
    params.init.unk_3 = 0;

    params.init.sizeDC0 = 0xDC0;
    std::vector<uint8_t> buf_dc0(params.init.sizeDC0);
    params.init.addr_DC0 = (ipmi_download_param*)buf_dc0.data();

    params.size2E0 = 0x2E0;
    std::vector<uint8_t> buf_2e0(params.size2E0);
    params.addr_2E0 = buf_2e0.data();

    params.pBgdlId = (uint32_t*)&bgdlid; // points to -1
    params.unk_5 = 4;
    params.result = &result;
    params.shell_func_8 = (*(sceDownloadObj->class_header->func_table))[8];

    if(!bgdl_payload_build(params.init.addr_DC0,type,title,url,rif,icon.c_str()))throw std::runtime_error("Invalid BGDL payload metadata");

    int res = sceDownloadObj->change_state(
            sceDownloadObj->class_header->func_table,
            0x12340012,
            params.init.ptr_to_dc0_ptr,
            1,
            params);

    if (res < 0)
        throw formatEx<std::runtime_error>(
                "SceDownload change_state failed: {:#08x}",
                static_cast<uint32_t>(res));
    if (result < 0)
        throw formatEx<std::runtime_error>(
                "SceDownload change_state result failed: {:#08x}",
                static_cast<uint32_t>(result));
    if (bgdlid < 0)
        throw formatEx<std::runtime_error>(
                "SceDownload change_state bgdlid failed: {:#08x}",
                static_cast<uint32_t>(res));
    result = 0;

    memset(&params, 0, sizeof(params));

    params.state.result = &result;
    params.state.unk_4 = 1;
    params.state.unk_7 = 0x00000A0A;

    res = sceDownloadObj->change_state(
            sceDownloadObj->class_header->func_table, 0x12340007, 0, 0, params);
    if (res < 0)
        throw formatEx<std::runtime_error>(
                "SceDownload second change_state failed: {:#08x}",
                static_cast<uint32_t>(res));
    if (result < 0)
        throw formatEx<std::runtime_error>(
                "SceDownload second change_state result failed: {:#08x}",
                static_cast<uint32_t>(result));
    buf_dc0.clear();
    buf_2e0.clear();
    return (unsigned)bgdlid;
}

std::unique_ptr<scedownload_class> new_scedownload()
{
    char lib_path[] = "vs0:sys/external/libshellsvc.suprx";

    sceKernelLoadStartModule(lib_path, 0, NULL, 0, NULL, NULL);

    int export1=taiGetModuleExportFunc(
            "SceShellSvc",
            0xF4E34EDB,
            0x4E255C31,
            (uintptr_t*)&SceIpmi_4E255C31);
    int export2=taiGetModuleExportFunc(
            "SceShellSvc",
            0xF4E34EDB,
            0xB282B430,
            (uintptr_t*)&SceIpmi_B282B430);

    if(export1<0 || export2<0 || !SceIpmi_4E255C31 || !SceIpmi_B282B430)throw std::runtime_error("ShellSvc exports unavailable; verify taiHEN and UNSAFE application");
    auto example_class = std::make_unique<scedownload_class>();
    init_download_class(example_class.get());
    return example_class;
}
static std::mutex bgdl_lock;
static std::unique_ptr<scedownload_class> download_class;
}

unsigned vs_start_bgdl(
        const int type,
        const std::string& title,
        const std::string& url,
        const std::vector<uint8_t>& rif, const std::string& icon)
{
    std::lock_guard<std::mutex> guard(bgdl_lock);
    if (pkgi_get_inode_type("ux0:bgdl/t")==InodeType::Directory && pkgi_list_dir_contents("ux0:bgdl/t").size() >= 32)
        throw std::runtime_error(
                "There are too many pending installation on your device, "
                "install them from LiveArea's notifications or delete them to "
                "be able to download more.");

    if(!download_class)download_class=new_scedownload();
    pkgi_mkdirs("ux0:data/VitaStation/bgdl");
    std::string license_path = "ux0:data/VitaStation/bgdl/license-"+std::to_string(sceKernelGetProcessTimeWide())+".rif";

    int rif_size = rif.size();
    if(rif_size >= PKGI_PSM_RIF_SIZE) {
        if (type == BgdlTypePsp)
            rif_size = PKGI_PSP_RIF_SIZE;
        else if(type == BgdlTypePsm)
            rif_size = PKGI_PSM_RIF_SIZE;
        else 
            rif_size = PKGI_RIF_SIZE;
    }
    
    if(rif_size>0)pkgi_save(license_path, rif.data(), rif_size);
    
    unsigned id=scedownload_start_with_rif(
            download_class.get(),
            title.c_str(),
            url.c_str(),
            (rif_size > 0) ? license_path.c_str() : "",
            type, icon);
    char message[128];snprintf(message,sizeof(message),"LiveArea accepted task id=%u type=%d",id,type);vs_log("bgdl",message,0);return id;
}
