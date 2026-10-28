#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "log.hpp"
#define pkgi_memcpy memcpy
#define pkgi_memequ(a,b,n) (memcmp(a,b,n)==0)
#define pkgi_snprintf snprintf
uint32_t pkgi_time_msec();
