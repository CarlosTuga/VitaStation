#include "../src/autoplugin_parser.hpp"
#include <cassert>
#include <cstdio>
int main(){auto p=ap_parse("Online_Plugins={\n--{ name = \"disabled\", path = \"bad.skprx\", section = \"KERNEL\" },\n{ name = \"ShellBat\", path = \"shellbat.suprx\", section = \"main\", crc = 0x12345678, v = \"1.0\" },\n{ name = \"Storage\", path = \"storagemgr.skprx\", section = \"KERNEL\" },\n{ name = \"invalid\", path = \"../escape.skprx\", section = \"KERNEL\" },\n}");assert(p.size()==2);assert(p[0].automatic && p[0].crc==0x12345678 && !strcmp(p[0].section,"*main"));assert(!p[1].automatic);puts("AutoPlugin metadata: no Lua execution, comments/path rejection and simple-install versus special-install classification passed.");}
