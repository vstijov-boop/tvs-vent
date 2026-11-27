#pragma once
// Važna upozorenja u dijalogu (user 2026-11-27: "vrlo rado bih ta važna upozorenja vidio u nekom
// dijalogu koji iskoči na ekran"): a command collects them with add() while it prints them in the
// command line as before; show() puts all of them in one message box at the end (nothing to show:
// no box). Used by TVS_PROTOK and TVS_PAD: protok usisa / potisa ventilatora, krajnji elementi bez
// protoka, protoci u crtežu nisu ažurni, ventilator u mreži a početak nije ventilator.
#include <string>
#include <vector>
#include <cstdarg>
#include <cwchar>
namespace tvsWarn {
inline std::vector<std::wstring>& items(){static std::vector<std::wstring> v;return v;}
inline void clear(){items().clear();}
// printf-like; also printed in the command line
inline void add(const wchar_t* format,...){
    wchar_t text[1024];va_list args;va_start(args,format);vswprintf_s(text,format,args);va_end(args);
    acutPrintf(L"\n%ls",text);items().push_back(text);
}
inline void show(const wchar_t* title){
    if(items().empty())return;
    std::wstring all;for(size_t k=0;k<items().size();++k){if(k)all+=L"\n\n";all+=L"• ";all+=items()[k];}
    clear();
    MessageBoxW(adsw_acadMainWnd(),all.c_str(),title,MB_OK|MB_ICONWARNING|MB_SETFOREGROUND);
}
// clears at the start of a command, shows at its end (also on an early return)
struct Scope {const wchar_t* title;explicit Scope(const wchar_t* t):title(t){clear();}~Scope(){show(title);}};
}
