#include "update_transaction.h"
UpdateOutcome update_transaction(const UpdateBackend*b,int*code){
 if(code)*code=0;
 if(!b || !b->verify || !b->initialize || !b->promote || !b->restart || !b->verify(b->context,false) || !b->verify(b->context,true))return UPDATE_INVALID;
 int n=b->initialize(b->context);if(n<0){if(code)*code=n;return UPDATE_INSTALLER_ERROR;}
 n=b->promote(b->context,false);
 if(n<0){if(code)*code=n;return b->promote(b->context,true)>=0?UPDATE_RECOVERED:UPDATE_RECOVERY_PENDING;}
 n=b->restart(b->context);if(n<0){if(code)*code=n;return UPDATE_RESTART_FAILED;}
 return UPDATE_OK;
}
