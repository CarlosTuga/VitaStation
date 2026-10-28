#pragma once
#include <fmt/format.h>
#include <stdexcept>
void vs_engine_log(const char* message);
#define LOG(...) ((void)0)
#define LOGF(...) ((void)0)
#define LOGFW(...) ((void)0)
#define LOGFE(...) ((void)0)
#define LOG_ERR(...) ((void)0)
template <typename E=std::runtime_error,typename... Args> E formatEx(const char* s,Args&&... args){return E(fmt::format(fmt::runtime(s),std::forward<Args>(args)...));}
