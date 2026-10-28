#ifndef VS_BOOTSTRAP_H
#define VS_BOOTSTRAP_H
#include "library.h"
bool bootstrap_start(const Library*initial);
/* Bit1 library handoff, bit2 plugin preparation result. */
unsigned bootstrap_poll(Library*library,int*scan_result,char*plugins_message,size_t size);
bool bootstrap_busy(void);
void bootstrap_shutdown(void);
#endif
