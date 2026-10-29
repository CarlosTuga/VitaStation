#pragma once
#include "integrations.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cctype>
static inline std::string ap_field(const std::string&s,const char*key){auto at=s.find(std::string(key)+" =");if(at==std::string::npos)at=s.find(std::string(key)+"=");if(at==std::string::npos)return {};at=s.find('"',at);if(at==std::string::npos)return {};auto end=s.find('"',at+1);return end==std::string::npos?std::string{}:s.substr(at+1,end-at-1);}
static inline std::vector<OnlinePlugin>ap_parse(const std::string&lua){
 std::string clean;for(size_t at=0;at<lua.size();){auto end=lua.find('\n',at);if(end==std::string::npos)end=lua.size();auto line=lua.substr(at,end-at);auto start=line.find_first_not_of(" \t\r");if(start!=std::string::npos && line.compare(start,2,"--"))clean+=line+"\n";at=end+1;}
 std::vector<OnlinePlugin>items;for(size_t at=0;at<clean.size() && items.size()<256;){at=clean.find('{',at);if(at==std::string::npos)break;auto end=clean.find('}',at);if(end==std::string::npos)break;auto row=clean.substr(at,end-at+1);at=end+1;auto file=ap_field(row,"path"),section=ap_field(row,"section"),name=ap_field(row,"name"),version=ap_field(row,"v");if(version.empty())version=ap_field(row,"version");if(file.empty() || file.size()>90 || name.empty() || file.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.-")!=std::string::npos || file.find("..")!=std::string::npos)continue;
  OnlinePlugin p{};snprintf(p.name,sizeof(p.name),"%s",name.c_str());snprintf(p.file,sizeof(p.file),"%s",file.c_str());snprintf(p.section,sizeof(p.section),"*%s",section.c_str());snprintf(p.version,sizeof(p.version),"%s",version.c_str());auto crc=row.find("0x");if(crc!=std::string::npos)p.crc=(unsigned)strtoul(row.c_str()+crc,nullptr,16);
  auto lower=file;std::transform(lower.begin(),lower.end(),lower.begin(),[](unsigned char c){return (char)std::tolower(c);});p.automatic=(lower=="shellbat.suprx" || lower=="shellsecbat.suprx" || lower=="pngshot.suprx" || lower=="nosleep.skprx" || lower=="ioplus.skprx") && (section=="KERNEL" || section=="ALL" || section=="main");items.push_back(p);
 }return items;
}
