cmake_minimum_required(VERSION 3.16)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain")
project(CaratulasDeLoja C)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
add_executable(CoverInstaller src/main.c)
target_compile_options(CoverInstaller PRIVATE -O2 -Wall -Wextra)
find_library(PNG_LIBRARY NAMES png16 png REQUIRED)
target_link_libraries(CoverInstaller vita2d freetype ${PNG_LIBRARY} jpeg z m c SceAppMgr_stub SceDisplay_stub SceGxm_stub SceSysmodule_stub SceCtrl_stub ScePgf_stub ScePvf_stub SceCommonDialog_stub SceLibKernel_stub SceKernelThreadMgr_stub SceProcessmgr_stub SceIofilemgr_stub SceSysmem_stub)
vita_create_self(CoverInstaller.self CoverInstaller UNSAFE)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/param.sfo"
 COMMAND "${VITA_MKSFOEX}" -s APP_VER=01.04 -s TITLE_ID=VSCV00001 "Caratulas de loja" "${CMAKE_CURRENT_BINARY_DIR}/param.sfo"
 COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/fix_sfo_title.py" "${CMAKE_CURRENT_BINARY_DIR}/param.sfo"
 DEPENDS CoverInstaller.self tools/fix_sfo_title.py VERBATIM)
add_custom_target(package-metadata ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/param.sfo")

