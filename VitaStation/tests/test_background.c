#include "../src/library.c"
#include <assert.h>
int main(){char found[256]={0};find_livearea_background("assets",found,sizeof(found));assert(strstr(found,"unusual-background-name.png"));puts("LiveArea art: arbitrary background filename found by dimensions; small icon rejected.");}
