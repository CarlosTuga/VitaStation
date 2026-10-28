#ifndef VS_LIBRARY_CACHE_H
#define VS_LIBRARY_CACHE_H
#include "library.h"
bool library_cache_load(Library*library,const char*path);
bool library_cache_save(const Library*library,const char*path);
bool library_cache_rebuild_due(bool valid_cache,const char*counter_path);
void library_cache_reset_counter(const char*counter_path);
#endif
