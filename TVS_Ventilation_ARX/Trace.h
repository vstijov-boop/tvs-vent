#pragma once
// TVS_TRAG (dijagnostika rušenja, korisnik 24.11.): uključen trag upisuje u %TEMP%\TVS_trag.log
// kratku liniju na ulazu / izlazu iz TVS reaktora, natpisa i komandi spajanja - svaka linija se
// odmah predaje sistemu (fflush), pa poslije rušenja AutoCAD-a zadnja linija kaže gdje je stalo.
// Isključen: jedna provjera bool-a, ništa se ne piše.
#include "acutads.h"
#include <cstdio>
#include <cwchar>
namespace tvsTrace {
inline bool& on(){static bool b=false;return b;}
inline FILE*& file(){static FILE* f=NULL;return f;}
inline const wchar_t* path(){static wchar_t p[MAX_PATH+32]={0};if(!p[0]){GetTempPathW(MAX_PATH,p);wcscat_s(p,L"TVS_trag.log");}return p;}
inline void mark(const char* where,const char* what){
    if(!on())return;
    if(!file()){if(_wfopen_s(&file(),path(),L"a")!=0)file()=NULL;if(!file())return;}
    SYSTEMTIME t;GetLocalTime(&t);
    std::fprintf(file(),"%02d:%02d:%02d.%03d %s %s\n",t.wHour,t.wMinute,t.wSecond,t.wMilliseconds,where,what?what:"");
    std::fflush(file());
}
inline void mark(const char* where,const wchar_t* what){
    if(!on())return;char b[512]={0};if(what)WideCharToMultiByte(CP_UTF8,0,what,-1,b,sizeof(b)-1,NULL,NULL);mark(where,b);
}
struct Scope {
    const char* n;
    explicit Scope(const char* name):n(name){mark(n,"ulaz");}
    ~Scope(){mark(n,"izlaz");}
};
inline void toggle(){
    on()=!on();
    if(on()){mark("TVS_TRAG","ukljucen");acutPrintf(L"\nTVS_TRAG uključen: %ls (ponovi radnju koja ruši AutoCAD, pa pošalji ovaj fajl).",path());}
    else{if(file()){std::fclose(file());file()=NULL;}acutPrintf(L"\nTVS_TRAG isključen (%ls).",path());}
}
}
