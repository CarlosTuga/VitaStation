#include "update_transaction.h"
#include <assert.h>
#include <stdio.h>
typedef struct {int calls,init,promote,recover,restart;int invalid,init_error,install_error,rollback_error,restart_error;} Fake;
static bool verify(void*c,bool rollback){Fake*f=c;f->calls++;return !(f->invalid && rollback);}
static int init(void*c){Fake*f=c;f->init++;return f->init_error;}
static int promote(void*c,bool rollback){Fake*f=c;if(rollback){f->recover++;return f->rollback_error;}f->promote++;return f->install_error;}
static int restart(void*c){Fake*f=c;f->restart++;return f->restart_error;}
static UpdateOutcome run(Fake*f){UpdateBackend b={f,verify,init,promote,restart};int code;return update_transaction(&b,&code);}
int main(void){
 Fake ok={0};assert(run(&ok)==UPDATE_OK && ok.calls==2 && ok.promote==1 && ok.recover==0 && ok.restart==1);
 Fake bad={0};bad.invalid=1;assert(run(&bad)==UPDATE_INVALID && bad.init==0 && bad.promote==0);
 Fake init_failed={0};init_failed.init_error=-1;assert(run(&init_failed)==UPDATE_INSTALLER_ERROR && init_failed.promote==0);
 Fake recovered={0};recovered.install_error=-2;assert(run(&recovered)==UPDATE_RECOVERED && recovered.recover==1 && recovered.restart==0);
 Fake pending={0};pending.install_error=-2;pending.rollback_error=-3;assert(run(&pending)==UPDATE_RECOVERY_PENDING && pending.recover==1);
 Fake launch={0};launch.restart_error=-4;assert(run(&launch)==UPDATE_RESTART_FAILED && launch.promote==1 && launch.recover==0);
 puts("Production update transaction: verify both trees before install; success/restart, refusal, rollback and pending recovery passed with injected system calls.");
}
