#pragma once
// Numeric fields of the dialogs (review 2026-11b, proposal 4): one place that reads, shows and
// compares numbers. Pure C++ (no MFC / AutoCAD), ASCII; tests/test_numeric_field.cpp.
// The MFC side is TVS_Ventilation_ARX/DecimalText.h (DDX wrappers over these functions).
//
// Rules:
// - text -> number: decimal comma or point, spaces around are ignored; anything else ("abc",
//   "1,5 m", "inf", "0x10", "") is not a number. "-0" is 0.
// - number -> text: printf "%.*g" (significant digits) or "%.*f" (decimals), never "-0".
// - a field is "changed" only when its NUMBER differs from the number that was shown, within
//   a tolerance; the texts are never compared ("1.5" / "1,50" / "1.5000000001" are no change).
// - the value of an untouched field is kept exactly as it was read (no round trip through text).
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <string>
namespace tvsNumeric {

// Degrees per radian (the dialogs show angles in degrees, the elements keep radians).
inline double degPerRad(){return 180.0/3.14159265358979323846;}
inline double degrees(double radians){return radians*degPerRad();}
inline double radians(double degrees){return degrees/degPerRad();}

// Drawing units <-> what the dialog shows (mm: factor = mm per drawing unit, 1 / 10 / 1000).
inline double toDisplay(double value,double factor){return value*factor;}
inline double fromDisplay(double shown,double factor){return factor!=0?shown/factor:shown;}

// Text -> number. False (v untouched) when the text is not exactly one number.
inline bool parse(const wchar_t* text,double& v){
    if(!text)return false;
    std::wstring t(text);
    std::wstring::size_type a=0,b=t.size();
    while(a<b&&std::iswspace(t[a]))++a;
    while(b>a&&std::iswspace(t[b-1]))--b;
    t=t.substr(a,b-a);
    if(t.empty())return false;
    for(std::wstring::size_type k=0;k<t.size();++k){
        wchar_t& c=t[k];
        if(c==L',')c=L'.';
        else if(!((c>=L'0'&&c<=L'9')||c==L'.'||c==L'+'||c==L'-'||c==L'e'||c==L'E'))return false;
    }
    wchar_t* end=NULL;
    const double x=std::wcstod(t.c_str(),&end);
    if(!end||end==t.c_str()||*end!=0||!std::isfinite(x))return false;
    v=x==0?0.0:x;   // -0 -> 0
    return true;
}
inline bool parse(const std::wstring& text,double& v){return parse(text.c_str(),v);}

// Number -> text with `digits` significant digits ("%.15g": 315, 0.315, 89.95437384).
inline std::wstring formatG(double v,int digits){
    if(!std::isfinite(v))return std::wstring();
    if(v==0)v=0.0;
    if(digits<1)digits=1;
    if(digits>17)digits=17;
    wchar_t buffer[64];
    std::swprintf(buffer,sizeof(buffer)/sizeof(buffer[0]),L"%.*g",digits,v);
    return buffer;
}
// Number -> text with exactly `decimals` decimals ("%.2f": 2.75, 3.00); no "-0.00".
inline std::wstring format(double v,int decimals){
    if(!std::isfinite(v))return std::wstring();
    if(decimals<0)decimals=0;
    if(decimals>15)decimals=15;
    const double unit=std::pow(10.0,-decimals);
    if(std::fabs(v)<unit/2)v=0.0;
    wchar_t buffer[64];
    std::swprintf(buffer,sizeof(buffer)/sizeof(buffer[0]),L"%.*f",decimals,v);
    return buffer;
}
// Text that keeps a number exactly (internal values passed between the command and its dialog).
inline std::wstring exact(double v){return formatG(v,17);}

// Same number within tol: absolute below 1, relative above (1e-9 of 2.755 m or of 89.954 deg).
inline bool same(double a,double b,double tol=1e-9){
    const double scale=std::fabs(a)>1?std::fabs(a):1;
    return std::fabs(a-b)<=tol*scale;
}
inline bool changed(double before,double after,double tol=1e-9){return !same(before,after,tol);}

// --- one field of a dialog ---------------------------------------------------------------
// `stored` is the value the command passed in (drawing units / radians, exact text) or a text
// that is not a number (L"Razli..." for different values, empty). `scale` turns it into what is
// shown (mm factor, degPerRad(), 1), `digits` = significant digits shown.

// What the field shows: the number scaled, or the text as it is.
inline std::wstring show(const wchar_t* stored,double scale,int digits){
    double v=0;
    if(!parse(stored,v))return stored?std::wstring(stored):std::wstring();
    return formatG(v*scale,digits);
}
// After OK: true = the user typed a number that differs from the shown one; `value` is then the
// new stored value (typed / scale). False = keep the stored value as it was (untouched, or the
// field holds a text that is not a number).
inline bool edited(const wchar_t* stored,const wchar_t* typed,double scale,int digits,double& value,double tol=1e-9){
    double now=0;
    if(!parse(typed,now))return false;
    double before=0;
    if(parse(show(stored,scale,digits),before)&&same(before,now,tol))return false;
    value=scale!=0?now/scale:now;
    return true;
}
// Command side: `before` / `after` stored texts; true = write `value` (after differs by number).
inline bool differs(const wchar_t* before,const wchar_t* after,double& value,double tol=1e-9){
    double now=0;
    if(!parse(after,now))return false;   // text (different values, empty): nothing to write
    double was=0;
    if(parse(before,was)&&same(was,now,tol))return false;
    value=now;
    return true;
}
// Range check of a typed value (shown units): lo < v < hi, or lo <= v when lowIncluded.
inline bool inside(double v,double lo,double hi,bool lowIncluded){
    return (lowIncluded?v>=lo:v>lo)&&v<hi;
}
}
