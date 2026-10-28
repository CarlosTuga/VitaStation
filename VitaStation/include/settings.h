#ifndef VITA_SETTINGS_H
#define VITA_SETTINGS_H

#include "ui.h"
#include "library.h"

/* Caminhos de dados (pasta criada em settings_save / library_save_userdata). */
#if defined(__vita__)
#define VS_DATA_DIR "ux0:data/VitaStation"
#else
#define VS_DATA_DIR "vs-data" /* isolated by the host test runner */
#endif
#define VS_CONFIG_PATH   VS_DATA_DIR "/config.ini"
#define VS_USERDATA_PATH VS_DATA_DIR "/userdata.csv"


#ifdef __cplusplus
extern "C" {
#endif
void settings_ensure_dir(const char *dir_path);
void settings_load(UIContext *ui, const char *config_path);
bool settings_save(const UIContext *ui, const char *config_path);

bool library_save_userdata(const Library *lib, const char *path);
void library_load_userdata(Library *lib, const char *path);

#ifdef __cplusplus
}
#endif
#endif // VITA_SETTINGS_H
