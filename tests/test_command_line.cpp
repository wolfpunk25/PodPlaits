#include "../pod/command_line.h"
#include <cassert>
#include <string>
#include <vector>
int main(){
 CommandLine usb,wire;std::vector<std::string> a,b;
 auto receiveA=[&](char*s){a.push_back(s);};auto receiveB=[&](char*s){b.push_back(s);};
 for(char c:std::string("SET 0 "))usb.Feed(c,receiveA);
 for(char c:std::string("MOOD 2\r\nPING\n"))wire.Feed(c,receiveB);
 for(char c:std::string("500\n"))usb.Feed(c,receiveA);
 assert(a.size()==1&&a[0]=="SET 0 500");assert(b.size()==2&&b[0]=="MOOD 2"&&b[1]=="PING");
 for(char c:std::string(130,'X')+"\nPLAY 0\n\n")wire.Feed(c,receiveB);
 assert(b.size()==3&&b.back()=="PLAY 0");
 puts("PASS: independent fragmented USB/UART commands, CRLF and overflow recovery");
}
