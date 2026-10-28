#pragma once
#include <utility>
template<class F> struct VsScopeExit {F fn; ~VsScopeExit(){fn();}};
struct VsScopeBuilder {};
template<class F> VsScopeExit<F> operator+(VsScopeBuilder,F fn){return {std::move(fn)};}
#define VS_JOIN_(a,b) a##b
#define VS_JOIN(a,b) VS_JOIN_(a,b)
#define BOOST_SCOPE_EXIT_ALL(c) auto VS_JOIN(vs_scope_,__LINE__)=VsScopeBuilder()+[c]()
