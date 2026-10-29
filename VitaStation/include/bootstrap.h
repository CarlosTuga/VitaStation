#ifndef VS_BOOTSTRAP_H
#define VS_BOOTSTRAP_H
#include "library.h"
bool bootstrap_start(const Library*initial);
bool bootstrap_start_mode(const Library*initial,bool scan,bool prepare);
/* Bit1 library handoff, bit2 plugin preparation result. */
unsigned bootstrap_poll(Library*library,int*scan_result,char*plugins_message,size_t size);
bool bootstrap_busy(void);
/* Stop entering plugin preparation; true only when an active preparation must finish. */
bool bootstrap_begin_exit(void);
void bootstrap_shutdown(void);
#endif
