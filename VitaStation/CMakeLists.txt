cmake_minimum_required(VERSION 3.16)

# Alvo escolhido de forma explicita:
#   cmake -DVITA=ON ..   -> compila VitaStation.vpk para PS Vita (requer VitaSDK)
#   cmake ..             -> compila os testes de host (nao depende de VITASDK estar definido)
option(VITA "Compilar para PS Vita (requer VitaSDK)" OFF)

if(VITA)
    if(NOT DEFINED ENV{VITASDK})
        message(FATAL_ERROR "A variavel de ambiente VITASDK nao esta definida. Instale o VitaSDK e faca: export VITASDK=/caminho/para/vitasdk")
    endif()
    # O toolchain tem de ser definido ANTES de project(); sem ele compila-se para o PC.
    if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain file")
    endif()
endif()

project(VitaStation C)

set(CMAKE_C_STANDARD 99)

include_directories(include)

set(SOURCES
    src/log.c
    src/library.c
    src/psp_media.c
    src/installation_watch.c
    src/library_cache.c
    src/bootstrap.c
    src/texture.c
    src/theme.c
    src/render.c
    src/ui.c
    src/input.c
    src/audio_synth.c
    src/audio_queue.c
    src/audio.c
    src/launcher.c
    src/settings_menu.c
    src/settings.c
    src/cover_pack.c
    src/catalog.c
    src/dlc_plan.c
    src/json_parser.c
    src/store_host.c
    src/bgdl_payload.c
    src/store_ui.c
    src/system_controls.c
    src/system_info.c
    src/system_audit.c
    src/system_catalog.c
    src/plugins.c
    src/autoboot.c
    src/system_ui.c
)

if(VITA)
    # vita.cmake define vita_create_self / vita_create_vpk (NAO existe vita2d.cmake no VitaSDK)
    include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
    # Decode PEM at build time; device imports individual DER roots directly.
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/cacert.pem" "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_ca.py" "${CMAKE_CURRENT_SOURCE_DIR}/tools/tls_roots.json")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_ca.py" "${CMAKE_CURRENT_SOURCE_DIR}/assets/cacert.pem" "${CMAKE_CURRENT_BINARY_DIR}/ca_bundle.h" RESULT_VARIABLE VS_CA_RESULT)
    if(NOT VS_CA_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to embed HTTPS trust roots")
    endif()
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 17)
    add_executable(VitaStationBoot plugins/VitaStationBoot/main.c)
    set_target_properties(VitaStationBoot PROPERTIES LINK_FLAGS "-nostdlib")
    target_link_libraries(VitaStationBoot SceAppMgr_stub SceCtrl_stub SceIofilemgr_stub SceLibKernel_stub SceKernelThreadMgr_stub)
    set(VS_BOOT "${CMAKE_CURRENT_BINARY_DIR}/VitaStationBoot.suprx")
    add_custom_command(OUTPUT "${VS_BOOT}"
        COMMAND "${VITA_ELF_CREATE}" -e "${CMAKE_CURRENT_SOURCE_DIR}/plugins/VitaStationBoot/exports.yml" "$<TARGET_FILE:VitaStationBoot>" "${CMAKE_CURRENT_BINARY_DIR}/VitaStationBoot.velf"
        COMMAND "${VITA_MAKE_FSELF}" -c "${CMAKE_CURRENT_BINARY_DIR}/VitaStationBoot.velf" "${VS_BOOT}"
        DEPENDS VitaStationBoot plugins/VitaStationBoot/exports.yml VERBATIM)
    set(VS_BOOT_HASH "${CMAKE_CURRENT_BINARY_DIR}/boot_hash.h")
    add_custom_command(OUTPUT "${VS_BOOT_HASH}"
        COMMAND "${CMAKE_COMMAND}" "-DINPUT=${VS_BOOT}" "-DOUTPUT=${VS_BOOT_HASH}" -P "${CMAKE_CURRENT_SOURCE_DIR}/plugins/VitaStationBoot/hash.cmake"
        DEPENDS "${VS_BOOT}" VERBATIM)

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
    add_executable(VitaStation ${SOURCES} src/main.c src/store_vita.cpp src/plugins_vita.cpp src/psp_bubbles.cpp
        third_party/usagi/download.cpp third_party/usagi/bgdl.cpp third_party/usagi/aes128.cpp
        third_party/usagi/sha256.cpp third_party/usagi/zrif.cpp third_party/usagi/puff.c
        third_party/usagi/vitahttp.cpp third_party/usagi/vitafile.cpp)
    target_sources(VitaStation PRIVATE "${VS_BOOT_HASH}")
    target_include_directories(VitaStation PRIVATE third_party "${CMAKE_CURRENT_BINARY_DIR}")
    target_compile_definitions(VitaStation PRIVATE FMT_HEADER_ONLY)
    target_compile_options(VitaStation PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-O2;-Wall;-Wextra>)

    # libwebp so e necessaria nas versoes mais recentes da vita2d
    find_library(WEBP_LIBRARY webp)
    set(EXTRA_LIBS "")
    if(WEBP_LIBRARY)
        list(APPEND EXTRA_LIBS webp)
    endif()

    # Windows packages may provide libpng16.a without the libpng.a symlink.
    find_library(PNG_LIBRARY NAMES png16 png REQUIRED)
    find_library(BZIP2_LIBRARY NAMES bz2 REQUIRED)
    # Dependencias da vita2d (static) + stubs usados pelo codigo
    target_link_libraries(VitaStation
        vita2d ${EXTRA_LIBS} freetype ${PNG_LIBRARY} jpeg zip ${BZIP2_LIBRARY} curl ssl crypto lzma zstd z m c stdc++ pthread
        SceSysmem_stub SceKernelModulemgr_stub
        taihen_stub taihenModuleUtils_stub SceVshBridge_stub SceRegistryMgr_stub
        SceHttp_stub SceNet_stub SceNetCtl_stub SceSsl_stub ScePromoterUtil_stub
        SceDisplay_stub SceGxm_stub SceSysmodule_stub SceCtrl_stub
        ScePgf_stub ScePvf_stub SceCommonDialog_stub
        SceLibKernel_stub SceKernelThreadMgr_stub SceProcessmgr_stub SceIofilemgr_stub
        SceAVConfig_stub SceTouch_stub SceAudio_stub ScePower_stub SceAppMgr_stub SceAppUtil_stub SceRtc_stub SceSqlite_stub
    )

    # UNSAFE: necessario para ler ux0:app/ de outras aplicacoes e lanca-las.
    vita_create_self(VitaStation.self VitaStation UNSAFE)
    # Some SDK vita_create_vpk macros split FILE paths containing spaces.
    # Explicit VERBATIM commands keep Windows paths as individual arguments.
    set(VS_SFO "${CMAKE_CURRENT_BINARY_DIR}/VitaStation_param.sfo")
    set(VS_SELF "${CMAKE_CURRENT_BINARY_DIR}/VitaStation.self")
    set(VS_VPK "${CMAKE_CURRENT_BINARY_DIR}/VitaStation.vpk")
    set(VS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icon0.png")
    set(VS_LOGO "${CMAKE_CURRENT_SOURCE_DIR}/assets/brand-logo.png")
    set(VS_CA "${CMAKE_CURRENT_SOURCE_DIR}/assets/cacert.pem")
    file(GLOB VS_PLUGIN_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/plugins/*")
    file(GLOB VS_PLUGIN_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/plugin-sources/*")
    file(GLOB VS_ALIVE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/alive/*")
    file(GLOB_RECURSE VS_PSP_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/psp-runtime/*")
    set(VS_PLUGIN_ARGS "")
    foreach(psp_file IN LISTS VS_PSP_FILES)
        file(RELATIVE_PATH psp_name "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${psp_file}")
        list(APPEND VS_PLUGIN_ARGS -a "${psp_file}=assets/${psp_name}")
    endforeach()
    foreach(plugin_file IN LISTS VS_PLUGIN_FILES)
        get_filename_component(plugin_name "${plugin_file}" NAME)
        list(APPEND VS_PLUGIN_ARGS -a "${plugin_file}=assets/plugins/${plugin_name}")
    endforeach()
    foreach(plugin_file IN LISTS VS_PLUGIN_SOURCES)
        get_filename_component(plugin_name "${plugin_file}" NAME)
        list(APPEND VS_PLUGIN_ARGS -a "${plugin_file}=assets/plugin-sources/${plugin_name}")
    endforeach()
    foreach(alive_file IN LISTS VS_ALIVE_FILES)
        get_filename_component(alive_name "${alive_file}" NAME)
        list(APPEND VS_PLUGIN_ARGS -a "${alive_file}=assets/alive/${alive_name}")
    endforeach()
    add_custom_command(OUTPUT "${VS_SFO}"
        COMMAND "${VITA_MKSFOEX}" -s APP_VER=01.60 -s TITLE_ID=VTST00001 VitaStation "${VS_SFO}"
        DEPENDS "${VS_SELF}" VERBATIM
    )
    add_custom_command(OUTPUT "${VS_VPK}"
        COMMAND "${VITA_PACK_VPK}" -a "${VS_ICON}=sce_sys/icon0.png" -a "${VS_LOGO}=assets/brand-logo.png" -a "${VS_CA}=assets/cacert.pem" -a "${VS_BOOT}=assets/plugins/VitaStationBoot.suprx" ${VS_PLUGIN_ARGS} -s "${VS_SFO}" -b "${VS_SELF}" "${VS_VPK}.tmp"
        COMMAND "${CMAKE_COMMAND}" -E rename "${VS_VPK}.tmp" "${VS_VPK}"
        DEPENDS "${VS_SFO}" "${VS_SELF}" "${VS_ICON}" "${VS_LOGO}" "${VS_CA}" "${VS_BOOT}" ${VS_PLUGIN_FILES} ${VS_PLUGIN_SOURCES} ${VS_ALIVE_FILES} ${VS_PSP_FILES} VERBATIM
    )
    add_custom_target(VitaStation-vpk ALL DEPENDS "${VS_VPK}")
else()
    enable_testing()
    # Alvo de testes de host
    add_executable(vitastation_main ${SOURCES} src/main.c)
    add_executable(test_vitastation ${SOURCES} tests/test_vitastation.c)
    add_test(NAME core COMMAND test_vitastation)
    target_link_libraries(test_vitastation m)
    target_link_libraries(vitastation_main m)
endif()
