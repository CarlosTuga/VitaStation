#include "../src/autoplugin_parser.hpp"
#include <cassert>
#include <fstream>
#include <iterator>
#include <cstdio>
int main(int argc,char**argv){if(argc==2){std::ifstream f(argv[1]);std::string content((std::istreambuf_iterator<char>(f)),{});auto bundled=ap_parse(content);assert(bundled.size()>=100);bool simple=false;for(auto&p:bundled)if(p.automatic)simple=true;assert(simple);}
auto p=ap_parse("Online_Plugins={\n--{ name = \"disabled\", path = \"bad.skprx\", section = \"KERNEL\" },\n{ name = \"ShellBat\", path = \"shellbat.suprx\", section = \"main\", crc = 0x12345678, v = \"1.0\" },\n{ name = \"Storage\", path = \"storagemgr.skprx\", section = \"KERNEL\" },\n{ name = \"invalid\", path = \"../escape.skprx\", section = \"KERNEL\" },\n}");assert(p.size()==2);assert(p[0].automatic && p[0].crc==0x12345678 && !strcmp(p[0].section,"*main"));assert(!p[1].automatic);puts("AutoPlugin metadata: no Lua execution, comments/path rejection and simple-install versus special-install classification passed.");}
